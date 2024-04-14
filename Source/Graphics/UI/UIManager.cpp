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
        info.PipelineRenderingCreateInfo = info::CreatePipelineRenderingInfo(formats, {});
//        const VkFormat f[1] = {VK_FORMAT_B8G8R8A8_SRGB};
//        ImGui_ImplVulkanH_SelectSurfaceFormat(shiftContext.device->GetPhysicalDevice(), shiftBackBuffer.windowSurface->Get(), f, 1, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
        ImGui_ImplVulkan_Init(&info);

        ImGui_ImplVulkan_CreateFontsTexture();
        vkDeviceWaitIdle(shiftContext.device->Get());
        ImGui_ImplVulkan_DestroyFontsTexture();

        ImGui::StyleColorsDark();
    }

    void UIManager::Destroy() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void UIManager::BeginFrame(uint32_t currentFrame) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_MenuBar;
        ImGui::Begin("Shift - A Rendering Sandbox", NULL, window_flags);

        ImGui::Text("Welcome to"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0.81f, 0.62f, 1.0f, 1.0f), "Shift!");

        ImGui::Text("Shift is a rendering sandbox for computer graphics research.");
        ImGui::Text("To move and rotate the camera, press RBM (Right Mouse Button) anywhere on the screen, then:");
        ImGui::Text("\t> 'W' - Forwards");
        ImGui::Text("\t> 'S' - Backwards");
        ImGui::Text("\t> 'A' - Left");
        ImGui::Text("\t> 'D' - Right");
        ImGui::Text("\t> 'E' - Up");
        ImGui::Text("\t> 'Q' - Down");
        ImGui::Text("> Move the mouse to rotate the camera");
        ImGui::Text("> Mouse wheel to increase/decrease camera speed");
        ImGui::Text("> For more camera settings go to > Settings > Camera Settings");
        ImGui::Spacing();
        ImGui::Text("> To Open Shift's Tools go to > Tools > ...");

        uint32_t id = 0;
        for (auto& [sectionName, component]: m_toolComponents) {
            ImGui::PushID(id++);
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu(sectionName.c_str())) {
                    for (auto toolCompPtr: component) {
                        toolCompPtr->Item();
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            for (auto toolCompPtr: component) {
                toolCompPtr->Show(currentFrame);
            }

            ImGui::PopID();
        }
    }

    void UIManager::EndFrame(const shift::gfx::CommandBuffer &buffer) {
        ImGui::End();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer.Get(), VK_NULL_HANDLE);
    }

    void UIManager::RegisterToolComponent(UIWindowComponent* componentPtr, const std::string& sectionName) {
        m_toolComponents[sectionName].push_back(componentPtr);
    }
}