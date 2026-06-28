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

#include "Graphics/RHI/Common/Capabilities.hpp"
#include "Utility/Assertions.hpp"
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

        //! Backend bring-up hook: constructs the Vulkan instance/surface/device/
        //! descriptor allocator/swapchain. Every backend's RHILocal specialization is expected to expose this same signature.
        bool InitBackend(GLFWwindow* window, uint32_t width, uint32_t height,
                         const RHIAppInfo& appInfo, const RHIRequiredFeatures& features) {
            const uint32_t appVersion = VK_MAKE_VERSION(appInfo.appVersionMajor, appInfo.appVersionMinor, appInfo.appVersionPatch);
            const uint32_t engineVersion = VK_MAKE_VERSION(appInfo.engineVersionMajor, appInfo.engineVersionMinor, appInfo.engineVersionPatch);

            instance = Core::CreateUnique<VK::Instance>(appInfo.appName, appVersion, appInfo.engineName, engineVersion, features);
            CheckCritical(instance->IsValid(), "Failed to create VK instance!");
            surface = Core::CreateUnique<VK::WindowSurface>(instance->Get(), window);
            CheckCritical(surface->IsValid(), "Failed to create VK surface!");
            device = Core::CreateUnique<VK::Device>(*instance, surface->Get(), features);
            CheckCritical(device->IsValid(), "Failed to create VK device!");
            descLayoutCache.Init(device.get());
            descAllocator = Core::CreateUnique<VK::DescriptorAllocator>(device.get());
            swapchain = Core::CreateUnique<VK::Swapchain>(device.get(), surface.get(), width, height);
            CheckCritical(swapchain->IsValid(), "Failed to create VK swapchain!");

            return true;
        }
    };
} // Shift

#endif //SHIFT_RHIDATA_VK_HPP