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

    void UIManager::BeginFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::Begin("Dear ImGui Demo", NULL, window_flags);
        static bool a = false;
        static bool b = false;
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                ImGui::MenuItem("ASS", NULL, &b);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        if (b) {
            ImGui::Begin("ASS", &b);
            if (ImGui::CollapsingHeader("Geometry")) {
                ImGui::Checkbox("Visualize normals", &a);
            }
            ImGui::End();
        }
    }

    void UIManager::EndFrame(const shift::gfx::CommandBuffer &buffer) {
        ImGui::End();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer.Get(), VK_NULL_HANDLE);
    }
}