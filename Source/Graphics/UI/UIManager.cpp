//
// Created by otrush on 3/5/2024.
//
#include "UIManager.hpp"

#include <array>

namespace shift::gfx::ui {
    void UIManager::CreateImGuiContext() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void UIManager::InitImGuiForVulkan(
            const ShiftContext& shiftContext,
            const ShiftBackBuffer& shiftBackBuffer,
            const ShiftWindow& window,
            const DescriptorPool& pool
            ) {
        ImGui_ImplGlfw_InitForVulkan(window.GetHandle(), true);

        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = shiftContext.instance->Get();
        info.DescriptorPool = pool.Get();
        info.Device = shiftContext.device->Get();
        info.PhysicalDevice = shiftContext.device->GetPhysicalDevice();
        info.QueueFamily = *shiftContext.device->GetQueueFamilyIndices().graphicsFamily;
        info.Queue = shiftContext.device->GetGraphicsQueue();
        info.ImageCount = gutil::SHIFT_MAX_FRAMES_IN_FLIGHT;
        info.MinImageCount = gutil::SHIFT_MAX_FRAMES_IN_FLIGHT;
        info.UseDynamicRendering = true;

        std::array<VkFormat, 1> formats{shiftBackBuffer.swapchain->GetFormat()};
        info.PipelineRenderingCreateInfo = info::CreatePipelineRenderingInfo(formats, shiftBackBuffer.swapchain->GetDepthBufferFormat());
        ImGui_ImplVulkan_Init(&info);

        ImGui_ImplVulkan_CreateFontsTexture();
        vkDeviceWaitIdle(shiftContext.device->Get());
        ImGui_ImplVulkan_DestroyFontsTexture();
    }

    void UIManager::Destroy() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}