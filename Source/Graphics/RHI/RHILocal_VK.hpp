//
// Created by otrush on 10/29/2025.
//

#ifndef SHIFT_RHILOCAL_VK_HPP
#define SHIFT_RHILOCAL_VK_HPP

#include <algorithm>
#include <iterator>
#include <mutex>
#include <vector>

#include "Graphics/RHI/Vulkan/VKDevice.hpp"
#include "Graphics/RHI/Vulkan/VKInstance.hpp"
#include "Graphics/RHI/Vulkan/VKWindowSurface.hpp"
#include "Graphics/RHI/Vulkan/VKSwapchain.hpp"
#include "Graphics/RHI/Vulkan/VKCommandBuffer.hpp"
#include "Graphics/RHI/Vulkan/VKSemaphore.hpp"
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

        ///! =============== Queue ownership handoffs ================ !///

        //! A released image waiting for its acquire half, plus the timeline point the releasing
        //! submit signals.
        struct PendingAcquire {
            VK::QueueOwnershipHandoff handoff;
            VK::TimelineSemaphore* releaseSemaphore = nullptr;
            uint64_t releaseValue = 0;
        };

        //! Store the acquires from a CB globally into RHI local. Each CB (context) calls this at submit
        //! if handoffs is not empty
        void QueuePendingAcquires(std::vector<VK::QueueOwnershipHandoff> handoffs,
                                  VK::TimelineSemaphore* releaseSemaphore, uint64_t releaseValue) {
            std::lock_guard<std::mutex> lock(pendingAcquireMutex);
            pendingAcquires.reserve(pendingAcquires.size() + handoffs.size());
            for (VK::QueueOwnershipHandoff& handoff : handoffs) {
                pendingAcquires.push_back(PendingAcquire{std::move(handoff), releaseSemaphore, releaseValue});
            }
        }

        //! Inverse of QueuePendingAcquires => Put acquires from a global list into a local CB to complete handoff.
        //! RETURNS the timeline points this recording's submit must now wait on
        [[nodiscard]] std::vector<SubmitTimelinePayload> FlushPendingAcquiresIntoCB(VK::CommandBuffer& cb) {
            const uint32_t cbFamily = device->GetQueueFamilyIndex(cb.GetPoolType());

            std::vector<PendingAcquire> mine;
            {
                std::lock_guard<std::mutex> lock(pendingAcquireMutex);
                if (pendingAcquires.empty()) { return {}; }

                //! satble partition so we just cut out the needed data from the back
                const auto mineBegin = std::stable_partition(pendingAcquires.begin(), pendingAcquires.end(),
                    [cbFamily](const PendingAcquire& pending) { return pending.handoff.dstFamily != cbFamily; });

                mine.assign(std::make_move_iterator(mineBegin), std::make_move_iterator(pendingAcquires.end()));
                pendingAcquires.erase(mineBegin, pendingAcquires.end());
            }

            std::vector<SubmitTimelinePayload> requiredWaits;
            for (const PendingAcquire& pending : mine) {
                cb.AcquireQueueOwnership(pending.handoff);

                //! Collapace the waits so we only wait at the latest semaphore value
                const auto existing = std::ranges::find(requiredWaits, pending.releaseSemaphore,
                                                        &SubmitTimelinePayload::semaphore);
                if (existing != requiredWaits.end()) {
                    existing->value = std::max(existing->value, pending.releaseValue);
                } else {
                    requiredWaits.push_back(SubmitTimelinePayload{pending.releaseSemaphore, pending.releaseValue});
                }
            }
            return requiredWaits;
        }

        //! If the texture is retired, clear its handoffs
        void CancelPendingAcquires(const VK::Texture* texture) {
            std::lock_guard<std::mutex> lock(pendingAcquireMutex);
            std::erase_if(pendingAcquires, [texture](const PendingAcquire& pending) {
                return pending.handoff.texture == texture;
            });
        }

        //! Shutdown: entries own nothing, so dropping them is the whole teardown
        void ClearPendingAcquires() {
            std::lock_guard<std::mutex> lock(pendingAcquireMutex);
            pendingAcquires.clear();
        }

        std::vector<PendingAcquire> pendingAcquires;
        std::mutex pendingAcquireMutex;
    };
} // Shift

#endif //SHIFT_RHIDATA_VK_HPP