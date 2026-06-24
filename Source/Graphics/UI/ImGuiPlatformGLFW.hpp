//! ImGui PLATFORM backend (Dear ImGui imgui_impl_glfw).

#ifndef SHIFT_IMGUIPLATFORMGLFW_HPP
#define SHIFT_IMGUIPLATFORMGLFW_HPP

struct GLFWwindow;

namespace Shift::Editor {
    //! ImGui PLATFORM backend: wraps imgui_impl_glfw. Renderer-agnostic, knows nothing
    //! about the graphics API, only the GLFW window. Pairs with the per-backend ImGuiBackend
    //! so the same platform base is used across Vulkan/DX12/Metal.
    //! Stateless: imgui_impl_glfw keeps its own global state tied to the ImGui context.
    class ImGuiPlatform {
    public:
        //! ImGui_ImplGlfw_InitForOther: "Other" because GLFW only special-cases OpenGL (unless I do multi viewport which I will, KEEP IN MIND).
        //! Vulkan/DX12/Metal are all identical, so this stays renderer-agnostic.
        static void Init(GLFWwindow* window);
        static void Shutdown();
        static void NewFrame();
    };
} // Shift::Editor

#endif //SHIFT_IMGUIPLATFORMGLFW_HPP
