//!
//! ImGui RENDERER backend (Dear ImGui imgui_impl_vulkan) bridged to Shift RHI handles
//!

#ifndef SHIFT_VKIMGUIBACKEND_HPP
#define SHIFT_VKIMGUIBACKEND_HPP

#include "Graphics/RHI/RHIContext.hpp"

struct ImDrawData;

namespace Shift::VK {
    //! ImGui RENDERER backend: wraps imgui_impl_vulkan and the Shift RHI handles it needs.
    //! The core RHI never includes this only the editor/renderer do.
    //! Stateless: imgui_impl_vulkan keeps its own global state tied to the ImGui context.
    class ImGuiBackend {
    public:
        static void Init(const RHILocal<RHI::Vulkan>& local);
        static void Shutdown();
        static void BeginFrame();
        //! Record ImGui draw data into the given command buffer
        static void RenderDrawData(ImDrawData* drawData, CommandBuffer& cmd);
        //! Register a sampled texture for display inside ImGui, returns an ImTextureID.
        [[nodiscard]] static void* RegisterTexture(const Texture& texture, const Sampler& sampler);
        //! Release a texture previously registered via RegisterTexture()
        static void UnregisterTexture(void* textureId);
    };
} // Shift::VK

#endif //SHIFT_VKIMGUIBACKEND_HPP
