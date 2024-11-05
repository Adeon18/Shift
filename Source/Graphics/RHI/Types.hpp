//
// Created by otrush on 10/6/2024.
//

#ifndef SHIFT_TYPES_HPP
#define SHIFT_TYPES_HPP

namespace Shift {
#ifdef SHIFT_VULKAN_BACKEND
    class Shader_VK;
    class Pipeline_VK;
    class Buffer_VK;
    class Texture_VK;
    class ResourceSet_VK;
    class Fence_VK;
    class Semaphore_VK;
    class Swapchain_VK;
    class Sampler_VK;
    class RenderPass_VK;
    class CommandBuffer_VK;

    using Shader = Shader_VK;
    using Pipeline = Pipeline_VK;
    using Buffer = Buffer_VK;
    using Texture = Texture_VK;
    using ResourceSet = ResourceSet_VK;
    using Fence = Fence_VK;
    using Semaphore = Semaphore_VK;
    using Swapchain = Swapchain_VK;
    using Sampler = Sampler_VK;
    using RenderPass = RenderPass_VK;
    using CommandBuffer = CommandBuffer_VK;
#endif

} // Shift

#endif //SHIFT_TYPES_HPP
