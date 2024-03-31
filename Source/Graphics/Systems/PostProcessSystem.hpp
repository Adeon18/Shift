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
    enum class ToneMapPass {
        Reinhard_ToneMapping = 0,
        LumaReinhard_ToneMapping = 1,
        RomBin_ToneMapping = 2,
        Uncharted2_ToneMapping = 3,
        ACES_ToneMapping = 4,
        Count = 5 // Total count
    };

    class PostProcessSystem {
        class UI: public ui::UIToolComponent {
        public:
            explicit UI(std::string name, PostProcessSystem& system): ui::UIToolComponent(std::move(name)), m_system{system} {
                ui::UIManager::GetInstance().RegisterToolComponent(this);
            }

            virtual void Item() override { ui::UIToolComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;

            //! Reinhard by default
            ToneMapPass chosenOperator = ToneMapPass::ACES_ToneMapping;
        private:
            //! Indices have to be equal to the ones in ToneMapPass
            std::array<const char*, static_cast<size_t>(ToneMapPass::Count)> m_toneMapOperatorNames{
                "Reinhard",
                "Reinhard Luma",
                "Rom Bin Da House",
                "Uncharted 2",
                "ACES"
            };

            PostProcessSystem& m_system;
        };
    public:
        PostProcessSystem(
                const Device& device,
                const ShiftBackBuffer& backBufferData,
                const SamplerManager& samplerManager,
                DescriptorManager &descManager,
                RenderTargetSystem& RTSystem);

        void ToneMap(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame);

        void ProcessResize();

        ~PostProcessSystem() = default;

        PostProcessSystem() = delete;
        PostProcessSystem(const PostProcessSystem&) = delete;
        PostProcessSystem& operator=(const PostProcessSystem&) = delete;
    private:
        //! UI
        UI m_UI{"Post Process System", *this};

        //! Create all the render stages
        void CreateRenderStages();

        //! Create the descriptor layouts used by the render stages
        void CreateDescriptorLayouts();

        const Device& m_device;
        const ShiftBackBuffer& m_backBufferData;
        const SamplerManager& m_samplerManager;
        DescriptorManager& m_descriptorManager;
        RenderTargetSystem& m_RTSystem;

        //! Single for now
        SGUID m_postProcessSetGuid;

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
                {ToneMapPass::RomBin_ToneMapping,
                     {
                             .name = "RomBin_ToneMapping",
                             .shaderData = {"FullscreenTriangle.vert.spv", "RomBinDaHouseToneMap.frag.spv", "", "", ""},
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
        };
    };
}

#endif //SHIFT_POSTPROCESSSYSTEM_HPP
