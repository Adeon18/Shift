//
// Created by otrush on 10/29/2025.
//

#ifndef SHIFT_RHILOCAL_VK_HPP
#define SHIFT_RHILOCAL_VK_HPP

#include "Graphics/RHI/Vulkan/VKDevice.hpp"
#include "Graphics/RHI/Vulkan/VKInstance.hpp"
#include "Graphics/RHI/Vulkan/VKWindowSurface.hpp"
#include "Graphics/RHI/Vulkan/Assistants/CommandPoolStorage.hpp"
#include "Graphics/RHI/Vulkan/Assistants/DescriptorLayoutCache.hpp"
#include "Graphics/RHI/Vulkan/Assistants/DescriptorAllocator.hpp"

namespace Shift {
    //! Note, this should be included only after both RHI Data and RHI::VUlkan have been defined
    template<> struct RHILocal<RHI::Vulkan> {
        VK::Instance instance;
        VK::Device device;
        VK::WindowSurface surface;

        VK::DescriptorAllocator descAllocator;
        VK::DescriptorLayoutCache descLayoutCache;
        VK::CommandPoolStorage cmdPoolStorage;
    };
} // Shift

#endif //SHIFT_RHIDATA_VK_HPP