//
// Created by otrush on 3/26/2024.
//

#ifndef SHIFT_POSTPROCESSSYSTEM_HPP
#define SHIFT_POSTPROCESSSYSTEM_HPP

#include "RenderStage.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Systems/TextureSystem.hpp"
#include "Graphics/Systems/ModelManager.hpp"
#include "Graphics/Systems/RenderTargetSystem.hpp"
#include "Graphics/Abstraction/Descriptors/BufferManager.hpp"
#include "Graphics/ShiftContextData.hpp"

namespace shift::gfx {
    enum class PostProcessPass {
        Reinhard_ToneMapping
    };

    class PostProcessSystem {
    public:
        PostProcessSystem(
                const Device& device,
                const ShiftBackBuffer& backBufferData,
                const SamplerManager& samplerManager,
                TextureSystem& textureSystem,
                ModelManager& modelManager,
                BufferManager& bufferManager,
                DescriptorManager &descManager,
                RenderTargetSystem& RTSystem);

        void ToneMap(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame);

        void ProcessResize();

        ~PostProcessSystem() = default;

        PostProcessSystem() = delete;
        PostProcessSystem(const PostProcessSystem&) = delete;
        PostProcessSystem& operator=(const PostProcessSystem&) = delete;
    private:
        //! Create all the render stages
        void CreateRenderStages();

        //! Create the descriptor layouts used by the render stages
        void CreateDescriptorLayouts();

        const Device& m_device;
        const ShiftBackBuffer& m_backBufferData;
        const SamplerManager& m_samplerManager;
        TextureSystem& m_textureSystem;
        ModelManager& m_modelManager;
        DescriptorManager& m_descriptorManager;
        BufferManager& m_bufferManager;
        RenderTargetSystem& m_RTSystem;

        //! Single for now
        SGUID m_postProcessSetGuid;

        std::unordered_map<PostProcessPass, RenderStage> m_postProcessStages;

        std::unordered_map<PostProcessPass, RenderStageCreateInfo> RENDER_STAGE_INFOS {
                {PostProcessPass::Reinhard_ToneMapping,
                        {
                                .name = "Reinhard_ToneMapping",
                                .shaderData = {"FullscreenTriangle.vert.spv", "ReinhardToneMap.frag.spv", "", "", ""},
                                .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                                .matSetLayoutType = MaterialSetLayoutType::POST_PROCESS,
                                .renderTargetType = RenderStageCreateInfo::RT_Type::Swapchain
                        }
                },
        };
    };
}

#endif //SHIFT_POSTPROCESSSYSTEM_HPP
