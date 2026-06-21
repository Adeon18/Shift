//
// Created by otrush on 1/7/2026.
//

#ifndef SHIFT_EDITORLAYER_HPP
#define SHIFT_EDITORLAYER_HPP

#include <vector>
#include <memory>
#include "EditorPanel.hpp"
#include "EditorContext.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_internal.h"
#include "ImGuiTools.hpp"
#include "Utility/Logging/LogMacros.hpp"

#include "Graphics/RHI/RHIContext.hpp"
#include "Window/ShiftWindow.hpp"

namespace Shift::Editor {

    class EditorLayer {
    public:
        EditorLayer() {
        }

        ~EditorLayer() {
        }

        template<typename API>
        void Init(const ShiftWindow& window, const RHILocal<API>& context);


        template<typename T, typename... Args>
        std::shared_ptr<T> AddPanel(Args&&... args) {
            auto panel = std::make_shared<T>(std::forward<Args>(args)...);
            m_Panels[panel->GetTitle()] = panel;
            return panel;
        }

        void BeginFrame() {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Create the invisible DockSpace covering the whole window
            ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport());

            static bool firstTime = true;
            if (firstTime) {
                firstTime = false;

                // 2. Clear any existing layout for this ID to ensure a clean slate
                ImGui::DockBuilderRemoveNode(dockspaceID);
                ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->Size);

                // 3. Split the Node
                // We start with the 'main' ID and split chunks off it.
                // The "Remaining" part of the split becomes the variable passed as the last argument.

                ImGuiID dock_main_id = dockspaceID;
                ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
                ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
                ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

                // 4. Dock Windows into the Nodes
                // "Viewport" goes into the center (dock_main_id)
                ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

                // Example: If you have a "Properties" panel, put it on the right
                ImGui::DockBuilderDockWindow("Properties", dock_id_right);

                // Example: "Scene Hierarchy" on the left
                ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left);

                // Example: "Content Browser" or "Console" on the bottom
                ImGui::DockBuilderDockWindow("Content Browser", dock_id_bottom);
                ImGui::DockBuilderDockWindow("Console", dock_id_bottom);

                // 5. Commit the layout
                ImGui::DockBuilderFinish(dockspaceID);
            }
        }

        void Render(void* engineTextureID) {
            m_Context.EngineOutputTextureID = engineTextureID;

            DrawMenuBar();

            // ImGui::ShowDemoWindow();

            for (auto& [_, panel] : m_Panels) {
                if (panel->IsOpen()) {
                    panel->IsVisible() = ImGui::Begin(panel->GetTitle().c_str(), &panel->IsOpen());
                    panel->OnImGuiRender();
                    ImGui::End();
                }
            }
        }

        void EndFrame() {
            ImGui::Render();
        }

        void RenderFloatingViewPorts() {
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
        }

        EditorContext& GetContext() { return m_Context; }
        ImDrawData* GetDrawData() { return ImGui::GetDrawData(); }
        bool ShouldRenderViewportPanel() { return m_Panels["Viewport"]->IsVisible() && m_Panels["Viewport"]->IsOpen(); }

        void Destroy() {
            ImGui_ImplVulkan_Shutdown();

            ImGui_ImplGlfw_Shutdown();

            ImGui::DestroyContext();
        }

    private:
        EditorContext m_Context;
        std::unordered_map<std::string, std::shared_ptr<EditorPanel>> m_Panels;

        void DrawMenuBar() {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("Panels")) {
                    for (auto& [name, panel] : m_Panels) {
                        if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, panel->IsOpen()))
                            panel->IsOpen() = !panel->IsOpen();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
        }
    };

    template<>
    inline void EditorLayer::Init<RHI::Vulkan>(const ShiftWindow& window, const RHILocal<RHI::Vulkan>& context) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // // Add this for debugging:
        // io.ConfigDebugBeginReturnValueOnce = true;
        // io.ConfigDebugBeginReturnValueLoop = true;

        ImGui::GetStyle().WindowRounding = 0.0f;

        ImGui_ImplGlfw_InitForVulkan(window.GetHandle(), true);

        ImGui_ImplVulkan_InitInfo info = {};
        info.Instance       = context.instance->Get();
        info.PhysicalDevice = context.device->GetPhysicalDevice();
        info.Device         = context.device->Get();
        info.QueueFamily    = *context.device->GetQueueFamilyIndices().graphicsFamily;
        info.Queue          = context.device->GetGraphicsQueue();
        info.PipelineCache  = VK_NULL_HANDLE;
        info.DescriptorPool = context.descAllocator->GetImGuiPool();

        //! TODO [BUG] This shit
        const uint32_t swapchainImageCount = static_cast<uint32_t>(context.swapchain->GetImages().size());
        info.MinImageCount = swapchainImageCount;
        info.ImageCount    = swapchainImageCount;

        // Validation/Error Checking
        info.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS) Log(Error, "ImGui Vulkan Error: {}\n", static_cast<uint32_t>(err));
        };

        info.UseDynamicRendering = true;

        static VkFormat mainColorFormat = VK::Util::ShiftToVKTextureFormat(context.swapchain->GetFormat());

        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &mainColorFormat;

        static VkFormat viewportColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

        info.PipelineInfoForViewports.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PipelineInfoForViewports.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        info.PipelineInfoForViewports.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoForViewports.PipelineRenderingCreateInfo.pColorAttachmentFormats = &viewportColorFormat;

        ImGui_ImplVulkan_Init(&info);

        ImGui::StyleColorsDark();
    }
}

#endif //SHIFT_EDITORLAYER_HPP