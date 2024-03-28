//
// Created by otrush on 3/23/2024.
//

#ifndef SHIFT_RENDERTARGETSYSTEM_HPP
#define SHIFT_RENDERTARGETSYSTEM_HPP

#include "Graphics/Abstraction/Device/Device.hpp"

#include "Graphics/Abstraction/Images/Images.hpp"
#include "Graphics/Abstraction/Images/SamplerManager.hpp"

#include "Graphics/Abstraction/Descriptors/DescriptorManager.hpp"

#include "Graphics/UI/UIWindowComponent.hpp"
#include "Graphics/UI/UIManager.hpp"

#include "Utility/Vulkan/UtilVulkan.hpp"

namespace shift::gfx {
    class RenderTargetSystem {
        class UI: public ui::UIToolComponent {
        public:
            explicit UI(std::string name, RenderTargetSystem& system): ui::UIToolComponent(std::move(name)), m_system{system} {
                ui::UIManager::GetInstance().RegisterToolComponent(this);
            }

            virtual void Item() override { ui::UIToolComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;

            std::unordered_map<SGUID, SGUID> textureIdToDescriptorIdLUT;
        private:
            RenderTargetSystem& m_system;
        };
    public:
        inline static std::string HDR_BUFFER = "HDR:RTF16";
        inline static std::string SWAPCHAIN_DEPTH = "Swapchain:Depth";

        RenderTargetSystem(const Device& device, const SamplerManager& samplerManager, DescriptorManager& descriptorManager);

        //! Create a render target and store it by name
        SGUID CreateRenderTarget2D(uint32_t width, uint32_t height, VkFormat format, const std::string& name);
        //! Create a render target and store it by name
        SGUID CreateDepthTarget2D(uint32_t width, uint32_t height, VkFormat format, const std::string& name);

        //! Check whether the ID of the RT is valid, or it was freed/recreated
        [[nodiscard]] bool IsValid(SGUID id);

        //! Get the RT id by name for tracking the
        [[nodiscard]] SGUID IdByName(const std::string& name);

        ColorRenderTerget2D& GetColorRT(SGUID id);
        ColorRenderTerget2D& GetColorRT(const std::string& name);

        DepthRenderTerget2D& GetDepthRT(SGUID id);
        DepthRenderTerget2D& GetDepthRT(const std::string& name);

        ~RenderTargetSystem() = default;

        RenderTargetSystem() = delete;
        RenderTargetSystem(const RenderTargetSystem&) = delete;
        RenderTargetSystem& operator=(const RenderTargetSystem&) = delete;
    private:
        //! UI component
        UI m_UI{"Render Target System", *this};

        const Device& m_device;
        const SamplerManager& m_samplerManager;
        DescriptorManager& m_descriptorManager;

        std::unordered_map<std::string, SGUID> m_RTNameToId;
        std::unordered_map<std::string, SGUID> m_DTNameToId;
        std::unordered_map<SGUID, std::unique_ptr<ColorRenderTerget2D>> m_renderTargets;
        std::unordered_map<SGUID, std::unique_ptr<DepthRenderTerget2D>> m_depthTargets;
    };
} // shift::gfx

#endif //SHIFT_RENDERTARGETSYSTEM_HPP
