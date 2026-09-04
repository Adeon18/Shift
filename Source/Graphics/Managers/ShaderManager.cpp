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

        //! Shader dir and shared structs
        const std::vector<std::string> includePaths{
            m_shaderSourceFolder.string(),
            Shift::Util::GetShiftGPUSharedDir(),
        };

#ifdef SHIFT_VULKAN_BACKEND
        m_compiler.Init(includePaths, Util::EShaderTarget::Vulkan_SPIRV);
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
        fallbackDescriptor.path = (m_shaderSourceFolder / "Fallback" / "Fallback.slang").string();

        const std::pair<EShaderType, const char*> fallbackStages[] = {
            { EShaderType::Vertex,   "mainVS" },
            { EShaderType::Fragment, "mainPS" },
        };

        for (const auto& [type, entry] : fallbackStages) {
            fallbackDescriptor.type = type;
            fallbackDescriptor.entry = entry;

            if (ShaderAsset* fallbackAsset = GetOrCreateAsset(fallbackDescriptor); fallbackAsset != nullptr) {
                m_fallbackShaders[type] = fallbackAsset;
            } else {
                //! No fallbacks availablke in this session - most likely not a crash but that is if I wrote good code:3
                LogError("Fallback {} shader failed to compile: {} - failures in this stage "
                            "will have no stand-in", ShaderTypeToString(type), fallbackDescriptor.path);
            }
        }
    }

    Shader* ShaderManager::GetFallbackShader(EShaderType type) {
        const auto it = m_fallbackShaders.find(type);
        return it == m_fallbackShaders.end() ? nullptr : it->second->shader;
    }

    ShaderResolution ShaderManager::GetShader(const ShaderDescriptor &desc) {
        const ShaderAsset* asset = GetOrCreateAsset(desc);
        return asset ? ShaderResolution{ asset->shader, asset->isFallback } : ShaderResolution{};
    }

    ShaderManager::ShaderAsset* ShaderManager::GetOrCreateAsset(const ShaderDescriptor &desc) {
        const std::string cacheKey = GetCacheKey(desc);

        if (const auto it = m_hashToShaderAsset.find(cacheKey); it != m_hashToShaderAsset.end()) {
            return it->second;
        }

        Core::UniquePtr<ShaderAsset> owned = Core::CreateUnique<ShaderAsset>();
        ShaderAsset* asset = owned.get();
        asset->descriptor = desc;

        if (LoadFromCacheAndRegister(cacheKey, asset) || CompileInternalAndRegister(cacheKey, asset)) {
            return owned.release();
        }

        //! Here we force the fallback bytecode under this shader key to have at least sopme shader
        const auto fallbackIt = m_fallbackShaders.find(desc.type);
        if (fallbackIt == m_fallbackShaders.end()) {
            LogError("Shader {} failed to compile and no fallback exists for its stage", cacheKey);
            return nullptr;
        }

        Shader* standIn = m_backend->CreateShader(fallbackIt->second->bytecode, asset->descriptor);
        if (!standIn || !standIn->IsValid()) {
            LogError("Could not build a fallback shader module for {}", cacheKey);
            delete standIn;
            return nullptr;
        }

        asset->shader = standIn;
        asset->isFallback = true;
        asset->bytecode.clear();
        m_hashToShaderAsset[cacheKey] = owned.release();

        UpdateDependencies(asset, { Shift::Util::NormalizePath(desc.path) });

        Log(Error, "Shader {} is serving FALLBACK bytecode until it compiles", cacheKey);
        return asset;
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

        std::unordered_set<ShaderAsset*> toRecompile;
        for (const auto& file: dirtyFiles) {
            const auto it = m_dependencyToShaders.find(file);
            if (it == m_dependencyToShaders.end()) { continue; }
            toRecompile.insert(it->second.begin(), it->second.end());
        }

        //! Retry anything depending on fallback
        for (const auto& [_, asset]: m_hashToShaderAsset) {
            if (asset->isFallback) { toRecompile.insert(asset); }
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
            //! A successful compile can still have plenty to say. Slang reports uninitialized
            //! variables and non-void functions that fall off the end as WARNINGS, and a shader
            //! carrying either produces undefined values with nothing else to notice - so they are
            //! worth seeing at the moment the shader compiles, not after chasing the artifact
            if (!result.errorLog.empty()) {
                Log(Warning, "Shader {} compiled with diagnostics:\n{}", key, result.errorLog);
            }

            asset->bytecode = result.data;
            if (m_hashToShaderAsset.contains(key)) {
                asset->shader->Rebuild(asset->bytecode);
                asset->isFallback = false;
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
