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

            std::unordered_map<SGUID, std::array<SGUID, gutil::SHIFT_MAX_FRAMES_IN_FLIGHT>> textureIdToDescriptorIdLUT;
        private:
            RenderTargetSystem& m_system;
        };
    public:
        RenderTargetSystem(const Device& device, const SamplerManager& samplerManager, DescriptorManager& descriptorManager);

        //! Create a render target and store it by name
        SGUID CreateRenderTarget2D(uint32_t width, uint32_t height, VkFormat format, const std::string& name);

        //! Check whether the ID of the RT is valid, or it was freed/recreated
        [[nodiscard]] bool IsValid(SGUID id);

        //! Get the RT id by name for tracking the
        [[nodiscard]] SGUID IdByName(const std::string& name);

        //! Store an existing VkImage by name
        SGUID RegisterRenderTarget2D(VkImage image, VkFormat format, std::string name);

        RenderTerget2D& GetRTCurrentFrame(SGUID id, uint32_t currentFrame);
        RenderTerget2D& GetRTPrevFrame(SGUID id, uint32_t currentFrame);
        RenderTerget2D& GetRTCurrentFrame(const std::string& name, uint32_t currentFrame);
        RenderTerget2D& GetRTPrevFrame(const std::string& name, uint32_t currentFrame);

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
        std::unordered_map<SGUID, std::array<std::unique_ptr<RenderTerget2D>, gutil::SHIFT_MAX_FRAMES_IN_FLIGHT>> m_renderTargets;
    };
} // shift::gfx

#endif //SHIFT_RENDERTARGETSYSTEM_HPP
