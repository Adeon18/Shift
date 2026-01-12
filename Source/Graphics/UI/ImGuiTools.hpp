//
// Created by otrush on 1/7/2026.
//

#ifndef SHIFT_IMGUITOOLS_HPP
#define SHIFT_IMGUITOOLS_HPP

#include "imgui/imgui.h"


#ifdef SHIFT_VULKAN_BACKEND

#include "imgui/imgui_impl_vulkan.h"
#include "Graphics/RHI/Vulkan/VKSampler.hpp"
#include "Graphics/RHI/Vulkan/VKTexture.hpp"
#include "Graphics/RHI/Vulkan/VKCommandBuffer.hpp"

namespace Shift {
    inline void * RegisterTextureForImGui(Sampler* sampler, Texture* texture) {
        return ImGui_ImplVulkan_AddTexture(sampler->VK_Get(), texture->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    inline void UnregisterTextureForImGui(void* descriptorSet) {
        if (descriptorSet) {
            ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(descriptorSet));
        }
    }

    inline void ImGuiRenderDrawData(ImDrawData* drawData, CommandBuffer* commandBuffer) {
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer->VK_Get());
    }
}
#endif

#endif //SHIFT_IMGUITOOLS_HPP