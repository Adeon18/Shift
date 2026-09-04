//
// Created by otrush on 11/26/2025.
//

#ifndef SHIFT_SHADERMANAGER_HPP
#define SHIFT_SHADERMANAGER_HPP

#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "Core/Memory.hpp"
#include "Graphics/RHI/RHI.hpp"
#include "Utility/SlangCompiler/SlangCompiler.hpp"

namespace Shift::Graphics {
    //! nullptr only when no fallback could replace the bytecode
    struct ShaderResolution {
        Shader* shader = nullptr;
        bool isFallback = false;
    };

    //! Owns shader compilation, caching and hot-reload bookkeeping
    class ShaderManager {
    public:
        void Init(RenderBackendInterface* backend, const std::string& shaderRootFolder);

        ShaderResolution GetShader(const ShaderDescriptor& desc);

        //! The stand-in shader for one stage
        Shader* GetFallbackShader(EShaderType type);

        //! Hot reload all relevant shaders and return the list of pipelines to be reloaded and deferred freed
        [[nodiscard]] std::unordered_set<Pipeline*> HotReload();

        //! Mark a shader source file as changed so the next HotReload() recompiles its dependents.
        //! Usually used just for testing as the file watcher auto detects the changes during runtime anywau
        void MarkDirty(const std::string& path);

        void RegisterPipeline(Shader* shader, Pipeline* pipeline);
        void UnregisterPipeline(Shader* shader, Pipeline* pipeline);

        void Destroy();

    private:
        struct ShaderAsset {
            std::vector<uint8_t> bytecode; //! Empty when isFallback
            ShaderDescriptor descriptor;
            std::unordered_set<std::string> dependencies;
            std::unordered_set<Pipeline*> subscribedPipelines;
            Shader* shader = nullptr;
            bool isFallback = false;
        };
        static std::string GetCacheKey(const ShaderDescriptor& desc);
        bool LoadFromCacheAndRegister(const std::string& key, ShaderAsset* asset);
        void SaveToCache(const std::string& key, const std::vector<uint8_t>& code, const std::unordered_set<std::string>& deps);
        bool CompileInternalAndRegister(const std::string& key, ShaderAsset* asset);
        void UpdateDependencies(ShaderAsset* asset, const std::unordered_set<std::string>& deps);
    private:
        RenderBackendInterface* m_backend = nullptr;
        Util::SlangCompiler m_compiler;

        //! Self explanatory lol
        ShaderAsset* GetOrCreateAsset(const ShaderDescriptor& desc);

        //! Hash -> ShaderAsset
        std::unordered_map<std::string, ShaderAsset*> m_hashToShaderAsset;

        //! 2. Dependency Path -> Shader assets that are influenced by it
        std::unordered_map<std::string, std::unordered_set<ShaderAsset*>> m_dependencyToShaders;

        std::unordered_set<std::string> m_dirtyFiles;
        std::mutex m_dirtyMutex;

        std::filesystem::path m_shaderRootFolder;
        std::filesystem::path m_shaderCacheFolder;
        std::filesystem::path m_shaderSourceFolder;

        std::unordered_map<EShaderType, ShaderAsset*> m_fallbackShaders;
    };
}

#endif //SHIFT_SHADERMANAGER_HPP