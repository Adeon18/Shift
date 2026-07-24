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
#include "imgui/imgui_internal.h"
#include "ImGuiPlatformGLFW.hpp"
#include "Utility/Logging/LogMacros.hpp"

#include "Graphics/RHI/RHIContext.hpp"
#include "Graphics/RHI/Vulkan/VKImGuiBackend.hpp"
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
            ImGuiBackend::BeginFrame();
            ImGuiPlatform::NewFrame();
            ImGui::NewFrame();

            //! Create the invisible DockSpace covering the whole window
            ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport());

            static bool firstTime = true;
            if (firstTime) {
                firstTime = false;

                //! Clear any existing layout for this ID to ensure a clean slate
                ImGui::DockBuilderRemoveNode(dockspaceID);
                ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->Size);

                //! Split the Node
                //! We start with the 'main' ID and split chunks off it.
                //! The "Remaining" part of the split becomes the variable passed as the last argument.

                ImGuiID dock_main_id = dockspaceID;
                ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
                ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
                ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

                //! Dock Windows into the Nodes
                //! "Viewport" goes into the center (dock_main_id)
                ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

                //! Right
                ImGui::DockBuilderDockWindow("Properties", dock_id_right);
                ImGui::DockBuilderDockWindow("GPU Timing", dock_id_right);

                //! Left
                ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left);

                //! Bottom
                ImGui::DockBuilderDockWindow("Content Browser", dock_id_bottom);
                ImGui::DockBuilderDockWindow("Console", dock_id_bottom);

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
            ImGuiBackend::Shutdown();

            ImGuiPlatform::Shutdown();

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

    template<typename API>
    void EditorLayer::Init(const ShiftWindow& window, const RHILocal<API>& context) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::GetStyle().WindowRounding = 0.0f;

        //! ImGui platform + renderer backends, both behind backend-agnostic
        //! names. EditorLayer itself stays free of of any API specific code
        ImGuiPlatform::Init(window.GetHandle());
        ImGuiBackend::Init(context);

        ImGui::StyleColorsDark();
    }
}

#endif //SHIFT_EDITORLAYER_HPP