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

//        ImGui::StyleColorsDark(); // Use the built-in dark theme as a base
//        // Get the current style
//        ImGuiStyle& style = ImGui::GetStyle();
//        // Modify specific colors for a yellow theme
//        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.737, 0.325, 0.949f, 1.0f); // Yellow title background
//        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 1.0f, 0.6f, 1.0f); // Brighter yellow title background when window is active
//        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.9f, 0.9f, 0.2f, 1.0f); // Darker yellow title background when window is collapsed
//        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8f, 0.8f, 0.0f, 1.0f); // Yellowish checkmark color
//        style.Colors[ImGuiCol_Button] = ImVec4(0.9f, 0.9f, 0.2f, 1.0f); // Yellow button color
//        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Lighter yellow button hover color
//        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.7f, 0.7f, 0.1f, 1.0f); // Darker yellow button pressed color

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

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Tools")) {
                for (auto toolCompPtr: m_toolComponents) {
                    toolCompPtr->Item();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        for (auto toolCompPtr: m_toolComponents) {
            toolCompPtr->Show();
        }
    }

    void UIManager::EndFrame(const shift::gfx::CommandBuffer &buffer) {
        ImGui::End();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer.Get(), VK_NULL_HANDLE);
    }

    void UIManager::RegisterToolComponent(UIToolComponent* componentPtr) {
        m_toolComponents.push_back(componentPtr);
    }
}