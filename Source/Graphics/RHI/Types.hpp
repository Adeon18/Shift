//
// Created by otrush on 10/6/2024.
//

#ifndef SHIFT_TYPES_HPP
#define SHIFT_TYPES_HPP

namespace Shift {
#ifdef SHIFT_VULKAN_BACKEND
    namespace VK {
        class Shader;
        class Pipeline;
        class Buffer;
        class Texture;
        class ResourceSet;
        class Fence;
        class Semaphore;
        class Swapchain;
        class Sampler;
        class RenderPass;
        class CommandBuffer;
        class Device;
    } // VK

    //! This is what is exported as an interface
    using Shader = VK::Shader;
    using Pipeline = VK::Pipeline;
    using Buffer = VK::Buffer;
    using Texture = VK::Texture;
    using ResourceSet = VK::ResourceSet;
    using Fence = VK::Fence;
    using Semaphore = VK::Semaphore;
    using Swapchain = VK::Swapchain;
    using Sampler = VK::Sampler;
    using RenderPass = VK::RenderPass;
    using CommandBuffer = VK::CommandBuffer;
#endif

} // Shift

#endif //SHIFT_TYPES_HPP
