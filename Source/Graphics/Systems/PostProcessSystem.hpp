//
// Created by otrush on 3/26/2024.
//

#ifndef SHIFT_POSTPROCESSSYSTEM_HPP
#define SHIFT_POSTPROCESSSYSTEM_HPP

#include "RenderStage.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Images/TextureManager.hpp"
#include "Graphics/Abstraction/Geometry/ModelManager.hpp"
#include "Graphics/Abstraction/Images/RenderTargetManager.hpp"
#include "Graphics/Abstraction/Descriptors/BufferManager.hpp"
#include "Graphics/ShiftContextData.hpp"

namespace Shift::gfx {
    enum class ToneMapPass {
        Reinhard_ToneMapping = 0,
        LumaReinhard_ToneMapping = 1,
        Lotes_ToneMapping = 2,
        Uncharted2_ToneMapping = 3,
        ACES_ToneMapping = 4,
        Disabled = 5,
        Count // Total count
    };

    class PostProcessSystem {
        class UI: public ui::UIWindowComponent {
        public:
            explicit UI(std::string name, std::string sName, PostProcessSystem& system): ui::UIWindowComponent{std::move(name), std::move(sName)}, m_system{system} {}

            virtual void Item() override { ui::UIWindowComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;

            //! Reinhard by default
            ToneMapPass chosenOperator = ToneMapPass::ACES_ToneMapping;

            bool exposureEnabled = true;
            bool gammaEnabled = true;
        private:
            //! Indices have to be equal to the ones in ToneMapPass
            std::array<const char*, static_cast<size_t>(ToneMapPass::Count)> m_toneMapOperatorNames{
                "Reinhard",
                "Reinhard Luma",
                "Lottes",
                "Uncharted 2",
                "ACES",
                "Disabled"
            };

            PostProcessSystem& m_system;
        };
    public:
        PostProcessSystem(
                const Device& device,
                const ShiftBackBuffer& backBufferData,
                const SamplerManager& samplerManager,
                DescriptorManager &descManager,
                BufferManager &bufManager,
                RenderTargetManager& RTSystem);

        void ToneMap(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame);

        void ProcessResize();

        ~PostProcessSystem() = default;

        PostProcessSystem() = delete;
        PostProcessSystem(const PostProcessSystem&) = delete;
        PostProcessSystem& operator=(const PostProcessSystem&) = delete;
    private:
        struct PostProcessUBO {
            // x - exposure, y enable exposure
            glm::vec4 data{1.1f, 1.0f, 0.0f, 0.0f};
        };

        //! UI
        UI m_UI{"Post Process System", "Tools", *this};

        //! Create all the render stages
        void CreateRenderStages();

        //! Create the descriptor layouts used by the render stages
        void CreateDescriptorLayouts();

        const Device& m_device;
        const ShiftBackBuffer& m_backBufferData;
        const SamplerManager& m_samplerManager;
        DescriptorManager& m_descriptorManager;
        BufferManager &m_bufManager;
        RenderTargetManager& m_RTSystem;

        //! Single for now
        SGUID m_postProcessSetGuid{};
        PostProcessUBO m_UBO;

        std::unordered_map<ToneMapPass, RenderStage> m_postProcessStages;

        std::unordered_map<ToneMapPass, RenderStageCreateInfo> RENDER_STAGE_INFOS {
                {ToneMapPass::Reinhard_ToneMapping,
                        {
                                .name = "Reinhard_ToneMapping",
                                .shaderData = {"FullscreenTriangle.vert.spv", "ReinhardToneMap.frag.spv", "", "", ""},
                                .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                                .matSetLayoutType = MaterialSetLayoutType::POST_PROCESS,
                                .renderTargetType = RenderStageCreateInfo::RT_Type::Swapchain
                        }
                },
                {ToneMapPass::LumaReinhard_ToneMapping,
                     {
                             .name = "LumaReinhard_ToneMapping",
                             .shaderData = {"FullscreenTriangle.vert.spv", "LumaReinhardToneMap.frag.spv", "", "", ""},
                             .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                             .matSetLayoutType = MaterialSetLayoutType::POST_PROCESS,
                             .renderTargetType = RenderStageCreateInfo::RT_Type::Swapchain
                     }
                },
                {ToneMapPass::Lotes_ToneMapping,
                     {
                             .name = "Lotes_ToneMapping",
                             .shaderData = {"FullscreenTriangle.vert.spv", "LotesToneMap.frag.spv", "", "", ""},
                             .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                             .matSetLayoutType = MaterialSetLayoutType::POST_PROCESS,
                             .renderTargetType = RenderStageCreateInfo::RT_Type::Swapchain
                     }
                },
                {ToneMapPass::Uncharted2_ToneMapping,
                     {
                             .name = "Uncharted2_ToneMapping",
                             .shaderData = {"FullscreenTriangle.vert.spv", "Uncharted2ToneMap.frag.spv", "", "", ""},
                             .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                             .matSetLayoutType = MaterialSetLayoutType::POST_PROCESS,
                             .renderTargetType = RenderStageCreateInfo::RT_Type::Swapchain
                     }
                },
                {ToneMapPass::ACES_ToneMapping,
                     {
                             .name = "ACES_ToneMapping",
                             .shaderData = {"FullscreenTriangle.vert.spv", "ACESToneMap.frag.spv", "", "", ""},
                             .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                             .matSetLayoutType = MaterialSetLayoutType::POST_PROCESS,
                             .renderTargetType = RenderStageCreateInfo::RT_Type::Swapchain
                     }
                },
                {ToneMapPass::Disabled,
                        {
                                .name = "None",
                                .shaderData = {"FullscreenTriangle.vert.spv", "DisabledToneMap.frag.spv", "", "", ""},
                                .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                                .matSetLayoutType = MaterialSetLayoutType::POST_PROCESS,
                                .renderTargetType = RenderStageCreateInfo::RT_Type::Swapchain
                        }
                },
        };
    };
}

#endif //SHIFT_POSTPROCESSSYSTEM_HPP
