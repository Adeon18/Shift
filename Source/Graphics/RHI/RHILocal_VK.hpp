//
// Created by otrush on 10/29/2025.
//

#ifndef SHIFT_RHILOCAL_VK_HPP
#define SHIFT_RHILOCAL_VK_HPP

#include "Graphics/RHI/Vulkan/VKDevice.hpp"
#include "Graphics/RHI/Vulkan/VKInstance.hpp"
#include "Graphics/RHI/Vulkan/VKWindowSurface.hpp"
#include "Graphics/RHI/Vulkan/VKSwapchain.hpp"
#include "Graphics/RHI/Vulkan/Assistants/DescriptorLayoutCache.hpp"
#include "Graphics/RHI/Vulkan/Assistants/DescriptorAllocator.hpp"

#include "Core/Memory.hpp"

namespace Shift {
    //! Note, this should be included only after both RHI Data and RHI::VUlkan have been defined
    template<> struct RHILocal<RHI::Vulkan> {
        Core::UniquePtr<VK::Instance> instance;
        Core::UniquePtr<VK::Device> device;
        Core::UniquePtr<VK::WindowSurface> surface;
        Core::UniquePtr<VK::Swapchain> swapchain;

        mutable Core::UniquePtr<VK::DescriptorAllocator> descAllocator;
        VK::DescriptorLayoutCache descLayoutCache;
    };
} // Shift

#endif //SHIFT_RHIDATA_VK_HPP