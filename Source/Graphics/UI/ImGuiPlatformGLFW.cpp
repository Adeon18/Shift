#include "ImGuiPlatformGLFW.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"

namespace Shift::Editor {

    void ImGuiPlatform::Init(GLFWwindow* window) {
        ImGui_ImplGlfw_InitForOther(window, true);
    }

    void ImGuiPlatform::Shutdown() {
        ImGui_ImplGlfw_Shutdown();
    }

    void ImGuiPlatform::NewFrame() {
        ImGui_ImplGlfw_NewFrame();
    }
} // Shift::Editor
