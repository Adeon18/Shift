//
// Created by otrush on 11/26/2025.
//
#include "SlangCompiler.hpp"
#include "Utility/UtilStandard.hpp"

namespace Shift::Graphics::Util {
    void SlangCompiler::Init(const std::vector<std::string> &includePaths, EShaderTarget targetPlatform) {
        m_targetPlatform = targetPlatform;
        slang::createGlobalSession(m_globalSession.writeRef());

        m_includePaths.reserve(includePaths.size());
        for (const auto& includePath : includePaths) {
            m_includePaths.push_back(includePath.c_str());
        }
    }

    ShaderCompileResult SlangCompiler::Compile(const std::filesystem::path &filePath, const std::string &entryPoint,
        EShaderType type) const
    {
        ShaderCompileResult result;

        //! --------- Cross platform target logic ---------
        slang::SessionDesc sessionDesc;
        slang::TargetDesc targetDesc;

        if (m_targetPlatform == EShaderTarget::Vulkan_SPIRV) {
            targetDesc.format = SLANG_SPIRV;
            targetDesc.profile = m_globalSession->findProfile("glsl_450");
            targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
        } else if (m_targetPlatform == EShaderTarget::DX12_DXIL) {
            targetDesc.format = SLANG_DXIL;
            targetDesc.profile = m_globalSession->findProfile("sm_6_5");
        }

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        //! Include paths for modules
        sessionDesc.searchPaths = m_includePaths.data();
        sessionDesc.searchPathCount = static_cast<SlangInt>(m_includePaths.size());

        //! Create Local Session
        Slang::ComPtr<slang::ISession> session;
        m_globalSession->createSession(sessionDesc, session.writeRef());

        //! --------- Load Module ---------
        Slang::ComPtr<slang::IBlob> diagnosticBlob;
        slang::IModule* module = session->loadModule(filePath.string().c_str(), diagnosticBlob.writeRef());

        if (diagnosticBlob) {
            result.errorLog += static_cast<const char*>(diagnosticBlob->getBufferPointer());
        }

        if (!module) {
            result.isValid = false;
            return result;
        }

        //! --------- Extract Dependencies (Useful for hot reload) ---------
        int depCount = module->getDependencyFileCount();
        for (int i = 0; i < depCount; i++) {
            // Returns absolute path usually
            const char* depPath = module->getDependencyFilePath(i);
            result.dependencies.emplace_back(Shift::Util::NormalizePath(depPath));
        }

        //! --------- Find entry point and link ---------
        Slang::ComPtr<slang::IEntryPoint> slangEntryPoint;
        module->findEntryPointByName(entryPoint.c_str(), slangEntryPoint.writeRef());

        if (!slangEntryPoint) {
            result.isValid = false;
            result.errorLog += "Entry point '" + entryPoint + "' not found.";
            return result;
        }

        //! --------- Combine module + entry point into a program ---------
        std::vector<slang::IComponentType*> components = { module, slangEntryPoint };
        Slang::ComPtr<slang::IComponentType> linkedProgram;

        session->createCompositeComponentType(
            components.data(),
            static_cast<SlangInt>(components.size()),
            linkedProgram.writeRef(),
            diagnosticBlob.writeRef()
        );

        if (diagnosticBlob) {
            result.errorLog += static_cast<const char*>(diagnosticBlob->getBufferPointer());
        }

        if (!linkedProgram) {
            result.isValid = false;
            return result;
        }

        //! --------- 5. Generate Bytecode (Codegen) ---------
        Slang::ComPtr<slang::IBlob> codeBlob;
        // Target Index 0 (since we only set one targetDesc)
        linkedProgram->getEntryPointCode(0, 0, codeBlob.writeRef(), diagnosticBlob.writeRef());

        if (diagnosticBlob) {
            result.errorLog += static_cast<const char*>(diagnosticBlob->getBufferPointer());
        }

        if (codeBlob) {
            result.isValid = true;
            // Copy blob to vector
            const uint8_t* ptr = static_cast<const uint8_t*>(codeBlob->getBufferPointer());
            size_t size = codeBlob->getBufferSize();
            result.data.assign(ptr, ptr + size);
        } else {
            result.isValid = false;
        }

        return result;
    }

    void SlangCompiler::Destroy() {
        m_globalSession.setNull();
    }
} // Shift::Graphics::Util