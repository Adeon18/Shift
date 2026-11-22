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
        class BinarySemaphore;
        class TimelineSemaphore;
        class Swapchain;
        class WindowSurface;
        class Sampler;
        class Instance;
        class CommandBuffer;
        class CommandPool;
        class Device;
        class ImGuiBackend;
    } // VK

    //! This is what is exported as an interface
    using Shader = VK::Shader;
    using Pipeline = VK::Pipeline;
    using Buffer = VK::Buffer;
    using Texture = VK::Texture;
    using ResourceSet = VK::ResourceSet;
    using TimelineSemaphore = VK::TimelineSemaphore;
    using Swapchain = VK::Swapchain;
    using Sampler = VK::Sampler;
    using CommandBuffer = VK::CommandBuffer;
    using CommandPool = VK::CommandPool;
    using Device = VK::Device;
    using ImGuiBackend = VK::ImGuiBackend;
#endif

#ifdef SHIFT_DX12_BACKEND
    namespace DX12 {
        class Shader;
        class Pipeline;
        class Buffer;
        class Texture;
        class ResourceSet;
        class TimelineSemaphore;
        class Swapchain;
        class Sampler;
        class CommandBuffer;
        class CommandPool;
        class Device;
    } // DX12

    using Shader = DX12::Shader;
    using Pipeline = DX12::Pipeline;
    using Buffer = DX12::Buffer;
    using Texture = DX12::Texture;
    using ResourceSet = DX12::ResourceSet;
    using TimelineSemaphore = DX12::TimelineSemaphore;
    using Swapchain = DX12::Swapchain;
    using Sampler = DX12::Sampler;
    using CommandBuffer = DX12::CommandBuffer;
    using CommandPool = DX12::CommandPool;
    using Device = DX12::Device;
#endif

} // Shift

#endif //SHIFT_TYPES_HPP
