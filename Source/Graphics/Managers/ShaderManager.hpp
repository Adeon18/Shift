//
// Created by otrush on 11/26/2025.
//

#ifndef SHIFT_SHADERMANAGER_HPP
#define SHIFT_SHADERMANAGER_HPP

#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "Graphics/RHI/Common/Types.hpp"
#include "Utility/SlangCompiler/SlangCompiler.hpp"

namespace Shift::Graphics {
    class ShaderManager {
    public:
        void Init(const Device* device, const std::string& shaderRootFolder);

        Shader* GetShader(const ShaderDescriptor& desc);

        //! Hot reload all relevant shaders and return the list of pipelines to be reloaded and deferred freed
        [[nodiscard]] std::unordered_set<Pipeline*> HotReload();

        void RegisterPipeline(Shader* shader, Pipeline* pipeline);
        void UnregisterPipeline(Shader* shader, Pipeline* pipeline);

        void Destroy();

    private:
        struct ShaderAsset {
            std::vector<uint8_t> bytecode;
            ShaderDescriptor descriptor;
            std::unordered_set<std::string> dependencies;
            std::unordered_set<Pipeline*> subscribedPipelines;
            Shader* shader = nullptr;
        };
        static std::string GetCacheKey(const ShaderDescriptor& desc);
        bool LoadFromCacheAndRegister(const std::string& key, ShaderAsset* asset);
        void SaveToCache(const std::string& key, const std::vector<uint8_t>& code, const std::unordered_set<std::string>& deps);
        bool CompileInternalAndRegister(const std::string& key, ShaderAsset* asset);
        void UpdateDependencies(ShaderAsset* asset, const std::unordered_set<std::string>& deps);
    private:
        const Device* m_device = nullptr;
        Util::SlangCompiler m_compiler;

        //! Hash -> ShaderAsset
        std::unordered_map<std::string, ShaderAsset*> m_hashToShaderAsset;

        //! 2. Dependency Path -> Shader assets that are influenced by it
        std::unordered_map<std::string, std::unordered_set<ShaderAsset*>> m_dependencyToShaders;

        std::unordered_set<std::string> m_dirtyFiles;
        std::mutex m_dirtyMutex;

        std::filesystem::path m_shaderRootFolder;
        std::filesystem::path m_shaderCacheFolder;
        std::filesystem::path m_shaderSourceFolder;

        std::unordered_map<EShaderType, std::string> m_fallbackShaders;
    };
}

#endif //SHIFT_SHADERMANAGER_HPP