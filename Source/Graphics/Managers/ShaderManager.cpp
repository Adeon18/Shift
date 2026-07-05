//
// Created by otrush on 11/26/2025.
//
#include "ShaderManager.hpp"

#include <fstream>

#include "Graphics/RHI/RHIContext.hpp"
#include "Utility/FileWatcher/FileWatcher.hpp"
#include "Utility/UtilStandard.hpp"

namespace Shift::Graphics {
    void ShaderManager::Init(RenderBackendInterface* backend, const std::string &shaderRootFolder) {
        m_backend = backend;
        m_shaderRootFolder = shaderRootFolder;
        m_shaderCacheFolder = std::filesystem::path(shaderRootFolder).parent_path() / "Build";
        if (!std::filesystem::exists(m_shaderCacheFolder)) {
            std::filesystem::create_directories(m_shaderCacheFolder);
        }
        m_shaderSourceFolder = std::filesystem::path(shaderRootFolder).parent_path() / "Source";

#ifdef SHIFT_VULKAN_BACKEND
        m_compiler.Init(std::vector{m_shaderSourceFolder.string()}, Util::EShaderTarget::Vulkan_SPIRV);
#endif


        auto& watcher = Shift::Util::FileWatcher::Get();
        watcher.AddWatch(m_shaderSourceFolder.string(),
                [this](const auto& path, auto action) {
                    //! We ignore any non-slang/temp files IDEs might create
                    if (!path.ends_with(".slang")) {
                        return;
                    }
                    if (action == Shift::Util::FileWatcher::EFileAction::Modified) {
                        MarkDirty(path);
                    }
                }
            );

        //! Get fallback shaders in case compilation crashes
        ShaderDescriptor fallbackDescriptor;
        fallbackDescriptor.entry = "mainVS";
        fallbackDescriptor.path = (m_shaderSourceFolder / "Fallback" / "Fallback.slang").string();
        fallbackDescriptor.type = EShaderType::Vertex;

        GetShader(fallbackDescriptor);
        m_fallbackShaders[EShaderType::Vertex] = GetCacheKey(fallbackDescriptor);

        fallbackDescriptor.entry = "mainPS";
        fallbackDescriptor.path = (m_shaderSourceFolder / "Fallback" / "Fallback.slang").string();
        fallbackDescriptor.type = EShaderType::Fragment;

        GetShader(fallbackDescriptor);
        m_fallbackShaders[EShaderType::Fragment] = GetCacheKey(fallbackDescriptor);
    }

    Shader* ShaderManager::GetShader(const ShaderDescriptor &desc) {
        std::string cacheKey = GetCacheKey(desc);

        if (m_hashToShaderAsset.contains(cacheKey)) {
            return m_hashToShaderAsset[cacheKey]->shader;
        }

        ShaderAsset* asset = new ShaderAsset();
        asset->descriptor = desc;

        if (LoadFromCacheAndRegister(cacheKey, asset)) {
            return m_hashToShaderAsset[cacheKey]->shader;
        }

        if (!CompileInternalAndRegister(cacheKey, asset)) {
            if (m_fallbackShaders.contains(asset->descriptor.type)) {
                Log(Warning, "Using fallback shader to not crash the application! Shader was not registered so hot reloading will not work!");
                return m_hashToShaderAsset[m_fallbackShaders[asset->descriptor.type]]->shader;
            }
        }

        return m_hashToShaderAsset[cacheKey]->shader;
    }

    void ShaderManager::MarkDirty(const std::string& path) {
        std::lock_guard lock{m_dirtyMutex};
        m_dirtyFiles.insert(Shift::Util::NormalizePath(path));
    }

    std::unordered_set<Pipeline*> ShaderManager::HotReload() {
        std::unordered_set<std::string> dirtyFiles;
        {
            std::lock_guard lock{m_dirtyMutex};
            if (m_dirtyFiles.empty()) { return {}; }
            dirtyFiles.swap(m_dirtyFiles);
        }

        std::vector<ShaderAsset*> toRecompile;
        for (const auto& file: dirtyFiles) {
            for (const auto& shader: m_dependencyToShaders[file]) {
                toRecompile.push_back(shader);
            }
        }

        std::unordered_set<Pipeline*> pipelinesToRebuild;
        for (const auto& shader: toRecompile) {
            bool success = CompileInternalAndRegister(GetCacheKey(shader->descriptor), shader);
            if (!success) { continue; }
            for (Pipeline* p: shader->subscribedPipelines) {
                pipelinesToRebuild.insert(p);
            }
        }
        return pipelinesToRebuild;
    }

    void ShaderManager::RegisterPipeline(Shader *shader, Pipeline *pipeline) {
        std::string key = GetCacheKey(shader->GetDesc());
        m_hashToShaderAsset[key]->subscribedPipelines.insert(pipeline);
    }

    void ShaderManager::UnregisterPipeline(Shader *shader, Pipeline *pipeline) {
        std::string key = GetCacheKey(shader->GetDesc());
        m_hashToShaderAsset[key]->subscribedPipelines.erase(pipeline);
    }

    void ShaderManager::Destroy() {
        // for (auto& [_, shader]: m_fallbackShaders) {
        //     m_hashToShaderAsset[shader]->shader->~Shader();
        // }
        for (auto& [_, shaderAsset]: m_hashToShaderAsset) {
            delete shaderAsset->shader;
            delete shaderAsset;
        }
    }

    std::string ShaderManager::GetCacheKey(const ShaderDescriptor &desc) {
        return std::filesystem::path(desc.path).filename().string() + "." + desc.entry + "." + ShaderTypeToString(desc.type) + "." + std::to_string(std::hash<std::string>{}(desc.path));
    }

    bool ShaderManager::LoadFromCacheAndRegister(const std::string &key, ShaderAsset *asset) {
        std::string path = (m_shaderCacheFolder /  key).string() + ".bin";
        if (!std::filesystem::exists(path)) return false;

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;

        //! Read Dependencies and timestamps
        uint32_t numDeps;
        file.read(reinterpret_cast<char *>(&numDeps), sizeof(uint32_t));

        std::unordered_set<std::string> deps;
        auto cacheTime = std::filesystem::last_write_time(path);

        for (uint32_t i = 0; i < numDeps; ++i) {
            uint32_t len;
            file.read(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            std::string depPath(len, ' ');
            file.read(depPath.data(), len);
            deps.insert(depPath);

            //! Is source newer than cached
            if (std::filesystem::exists(depPath)) {
                if (std::filesystem::last_write_time(depPath) > cacheTime) {
                    return false;
                }
            }
        }

        //! Read actual bytecode
        uint32_t size;
        file.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));
        asset->bytecode.resize(size);
        file.read(reinterpret_cast<char *>(asset->bytecode.data()), size);

        //! Create shader handle via the RHI primitive
        Shader* shader = m_backend->CreateShader(asset->bytecode, asset->descriptor);
        CheckCritical(shader->IsValid(), "Invalid shader!");
        asset->shader = shader;

        m_hashToShaderAsset[key] = asset;

        //! Recover dependencies for hot reloading
        UpdateDependencies(asset, deps);

        Log(Trace, "Loaded shader from cache: {}", path);

        return true;
    }

    void ShaderManager::SaveToCache(const std::string &key, const std::vector<uint8_t> &code,
        const std::unordered_set<std::string> &deps)
    {
        std::string path = (m_shaderCacheFolder / key).string() + ".bin";
        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        if (!file.is_open()) {
            Log(Error, "Failed to write cache file: {}", path);
            return;
        }

        //! Write Dependencies
        uint32_t numDeps = static_cast<uint32_t>(deps.size());
        file.write(reinterpret_cast<char *>(&numDeps), sizeof(uint32_t));
        for (const auto& d : deps) {
            uint32_t len = static_cast<uint32_t>(d.size());
            file.write(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            file.write(d.data(), len);
        }

        //! Write Bytecode
        uint32_t size = static_cast<uint32_t>(code.size());
        file.write(reinterpret_cast<char *>(&size), sizeof(uint32_t));
        if (size > 0) {
            file.write(reinterpret_cast<const char *>(code.data()), size);
        }

        //! Verify write
        if (file.fail()) {
            Log(Error, "I/O Error while writing cache: {}", path);
            // Delete partial file to prevent corruption on next read
            file.close();
            std::filesystem::remove(path);
        } else {
            Log(Trace, "Saved shader to cache: {}", path);
        }
    }

    bool ShaderManager::CompileInternalAndRegister(const std::string& key, ShaderAsset *asset) {
        Util::ShaderCompileResult result = m_compiler.Compile(asset->descriptor.path, asset->descriptor.entry, asset->descriptor.type);

        if (result.isValid) {
            asset->bytecode = result.data;
            if (m_hashToShaderAsset.contains(key)) {
                asset->shader->Rebuild(asset->bytecode);
                Log(Trace, "Hot-reload compiled shader {}", key);
            } else {
                //! Shader has not been registered - register it!
                Shader* shader = m_backend->CreateShader(asset->bytecode, asset->descriptor);
                CheckCritical(shader->IsValid(), "Invalid shader!");
                asset->shader = shader;
                m_hashToShaderAsset[key] = asset;
                Log(Trace, "Uncached shader! Compiled shader {}", key);
            }

            std::unordered_set<std::string> newDeps = std::unordered_set<std::string>{result.dependencies.begin(), result.dependencies.end()};

            UpdateDependencies(asset, newDeps);

            SaveToCache(key, asset->bytecode, asset->dependencies);
        } else {
            Log(Error, "Failed to compile shader {}: {}", key, result.errorLog);
            return false;
        }
        return true;
    }

    void ShaderManager::UpdateDependencies(ShaderAsset *asset, const std::unordered_set<std::string> &newDeps) {
        if (asset->dependencies == newDeps) { return; }

        std::vector<std::string> toAdd;

        for (const auto& dep: newDeps) {
            if (!asset->dependencies.contains(dep)) {
                toAdd.push_back(dep);
            } else {
                asset->dependencies.erase(dep);
            }
        }

        auto& toRemove = asset->dependencies;

        if (!toRemove.empty()) {
            for (const auto& dep: toRemove) {
                m_dependencyToShaders[dep].erase(asset);
                if (m_dependencyToShaders[dep].empty()) { m_dependencyToShaders.erase(dep); }
            }
        }
        if (!toAdd.empty()) {
            for (const auto& dep: toAdd) {
                m_dependencyToShaders[dep].insert(asset);
            }
        }

        asset->dependencies = newDeps;
    }
}
