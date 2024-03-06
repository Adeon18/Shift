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


namespace shift::gfx::ui {
    class UIManager {
    public:
        UIManager() = default;

        void CreateImGuiContext();

        void InitImGuiForVulkan(
                const ShiftContext& shiftContext,
                const ShiftBackBuffer& shiftBackBuffer,
                const ShiftWindow& window,
                const DescriptorPool& pool
                );

        void Destroy();
    private:
    };
} // shift::gfx::ui

#endif //SHIFT_UIMANAGER_HPP
