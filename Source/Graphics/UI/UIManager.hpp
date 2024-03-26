#ifndef SHIFT_UIMANAGER_HPP
#define SHIFT_UIMANAGER_HPP

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "Utility/Vulkan/InfoUtil.hpp"

#include "Graphics/ShiftContextData.hpp"
#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Device/Swapchain.hpp"
#include "Graphics/Abstraction/Descriptors/DescriptorManagement.hpp"
#include "Graphics/Abstraction/Commands/CommandBuffer.hpp"
#include "Window/ShiftWindow.hpp"

#include "UIWindowComponent.hpp"

namespace shift::gfx::ui {
    class UIManager {
    public:
        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;

        static UIManager& GetInstance() {
            static UIManager u;
            return u;
        }

        //! Create the general imgui context, is done first
        void CreateImGuiContext();

        //! Init vulkan and glfw implementations
        void InitImGuiForVulkan(
                const ShiftContext& shiftContext,
                const ShiftBackBuffer& shiftBackBuffer,
                const ShiftWindow& window,
                const DescriptorPool& pool
                );

        //! Begin the frame
        void BeginFrame(uint32_t currentFrame);

        void EndFrame(const CommandBuffer& buffer);

        void RegisterToolComponent(UIToolComponent* componentPtr);

        //! Destroy imgui context
        void Destroy();
    private:
        UIManager() = default;

        std::vector<UIToolComponent*> m_toolComponents;
    };
} // shift::gfx::ui

#endif //SHIFT_UIMANAGER_HPP
