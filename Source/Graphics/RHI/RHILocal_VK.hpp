//
// Created by otrush on 10/29/2025.
//

#ifndef SHIFT_RHILOCAL_VK_HPP
#define SHIFT_RHILOCAL_VK_HPP

#include <algorithm>
#include <array>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

#include "Graphics/RHI/Vulkan/VKDevice.hpp"
#include "Graphics/RHI/Vulkan/VKInstance.hpp"
#include "Graphics/RHI/Vulkan/VKWindowSurface.hpp"
#include "Graphics/RHI/Vulkan/VKSwapchain.hpp"
#include "Graphics/RHI/Vulkan/VKCommandBuffer.hpp"
#include "Graphics/RHI/Vulkan/VKSemaphore.hpp"
#include "Graphics/RHI/Vulkan/VKFence.hpp"
#include "Graphics/RHI/Vulkan/Assistants/DescriptorLayoutCache.hpp"
#include "Graphics/RHI/Vulkan/Assistants/DescriptorAllocator.hpp"

#include "Graphics/RHI/Common/Capabilities.hpp"
#include "Graphics/RHI/Common/Log.hpp"
#include "Graphics/RHI/Common/Memory.hpp"
#include "Graphics/RHI/Common/NativeWindow.hpp"
#include "Graphics/RHI/Common/RHIConfig.hpp"

namespace Shift {
    //! Note, this should be included only after both RHI Data and RHI::VUlkan have been defined
    template<> struct RHILocal<RHI::Vulkan> {
        RHI::UniquePtr<VK::Instance> instance;
        RHI::UniquePtr<VK::Device> device;
        RHI::UniquePtr<VK::WindowSurface> surface;
        RHI::UniquePtr<VK::Swapchain> swapchain;

        mutable RHI::UniquePtr<VK::DescriptorAllocator> descAllocator;
        VK::DescriptorLayoutCache descLayoutCache;

        //! Some docs as I always forget
        //! Acquire signals imageAvailable[frame slot] The submit writing that image waits it
        std::array<RHI::UniquePtr<VK::BinarySemaphore>, RHI::MAX_FRAMES_IN_FLIGHT> imageAvailable;
        std::vector<RHI::UniquePtr<VK::BinarySemaphore>> renderFinished;
        //! CPU wait on presentation
        std::vector<RHI::UniquePtr<VK::Fence>> presentFences;
        //! The imageAvailable the last acquire signalled, waited by the next presenting submit
        VK::BinarySemaphore* lastAcquireSemaphore = nullptr;

        //! This is wrong implementation, with this we can wait until pixels appear in screen but NOT for presentation
        // std::vector<uint64_t> m_imagePresentIds;
        // uint64_t m_globalPresentId = 0;

        //! Backend bring-up hook: constructs the Vulkan instance/surface/device/
        //! descriptor allocator/swapchain. Every backend's RHILocal specialization is expected to expose this same signature.
        bool InitBackend(const NativeWindow& window, uint32_t width, uint32_t height, const RHIAppInfo& appInfo, const RHIBindlessCapacity& bindless, const RHIRequiredFeatures& requiredFeatures) {
            const bool headless = window.platform == EPlatform::None;
            RHIRequiredFeatures features = requiredFeatures;
            if (headless) {
                features.VK_maintenance1 = false;
                features.VK_presentWait = false;
            }

            const uint32_t appVersion = VK_MAKE_VERSION(appInfo.appVersionMajor, appInfo.appVersionMinor, appInfo.appVersionPatch);
            const uint32_t engineVersion = VK_MAKE_VERSION(appInfo.engineVersionMajor, appInfo.engineVersionMinor, appInfo.engineVersionPatch);

            const std::vector<const char*> surfaceExtensions = VK::WindowSurface::GetInstanceExtensions(window.platform);
            instance = RHI::CreateUnique<VK::Instance>(appInfo.appName, appVersion, appInfo.engineName, engineVersion, features, surfaceExtensions);
            SRHI_CHECK_CRITICAL(instance->IsValid(), "Failed to create VK instance!");
            if (!headless) {
                surface = RHI::CreateUnique<VK::WindowSurface>(instance->Get(), window);
                SRHI_CHECK_CRITICAL(surface->IsValid(), "Failed to create VK surface!");
            }
            device = RHI::CreateUnique<VK::Device>(*instance, headless ? VK_NULL_HANDLE : surface->Get(), features);
            SRHI_CHECK_CRITICAL(device->IsValid(), "Failed to create VK device!");
            descLayoutCache.Init(device.get());
            descAllocator = RHI::CreateUnique<VK::DescriptorAllocator>(device.get(), bindless);
            //! Skip the evil and terrible
            if (headless) {
                return true;
            }

            swapchain = RHI::CreateUnique<VK::Swapchain>(device.get(), surface.get(), width, height);
            SRHI_CHECK_CRITICAL(swapchain->IsValid(), "Failed to create VK swapchain!");

            const uint32_t imageCount = static_cast<uint32_t>(swapchain->GetImages().size());
            for (uint32_t i = 0; i < imageCount; i++) {
                auto& sem = renderFinished.emplace_back(RHI::CreateUnique<VK::BinarySemaphore>(device.get()));
                SRHI_CHECK_CRITICAL(sem->IsValid(), "Failed to create render semaphore");
                sem->SetDebugName(("RenderFinished[" + std::to_string(i) + "]").c_str());
                auto& fence = presentFences.emplace_back(RHI::CreateUnique<VK::Fence>(device.get(), true));
                SRHI_CHECK_CRITICAL(fence->IsValid(), "Failed to create present wait fence");
                fence->SetDebugName(("PresentFence[" + std::to_string(i) + "]").c_str());
            }

            for (uint32_t i = 0; i < RHI::MAX_FRAMES_IN_FLIGHT; ++i) {
                imageAvailable[i] = RHI::CreateUnique<VK::BinarySemaphore>(device.get());
                SRHI_CHECK_CRITICAL(imageAvailable[i]->IsValid(), "Failed Init Acquire Sem");
                imageAvailable[i]->SetDebugName(("ImageAvailable[" + std::to_string(i) + "]").c_str());
            }

            return true;
        }

        bool RecreateSwapchain(uint32_t width, uint32_t height) {
            if (!swapchain->Recreate(width, height)) {
                return false;
            }

            const uint32_t newImageCount = static_cast<uint32_t>(swapchain->GetImages().size());

            if (renderFinished.size() != newImageCount) {
                renderFinished.clear();
                for (uint32_t i = 0; i < newImageCount; i++) {
                    auto& sem = renderFinished.emplace_back(RHI::CreateUnique<VK::BinarySemaphore>(device.get()));
                    SRHI_CHECK_CRITICAL(sem->IsValid(), "Failed to recreate render semaphore");
                    sem->SetDebugName(("RenderFinished[" + std::to_string(i) + "]").c_str());
                }
            }
            return true;
        }

        void DestroySwapchain() {
            swapchain.reset();
            for (auto& sem : imageAvailable) {
                sem.reset();
            }
            renderFinished.clear();
            presentFences.clear();
            lastAcquireSemaphore = nullptr;
        }

        ///! =============== Queue ownership handoffs ================ !///

        //! A released image waiting for its acquire half, plus the timeline point the releasing
        //! submit signals.
        struct PendingAcquire {
            QueueOwnershipHandoffPayload handoff;
            VK::TimelineSemaphore* releaseSemaphore = nullptr;
            uint64_t releaseValue = 0;
        };

        //! Store the acquires from a CB globally into RHI local. Each CB (context) calls this at submit
        //! if handoffs is not empty
        void QueuePendingAcquires(std::vector<QueueOwnershipHandoffPayload> handoffs,
                                  VK::TimelineSemaphore* releaseSemaphore, uint64_t releaseValue) {
            std::lock_guard<std::mutex> lock(pendingAcquireMutex);
            pendingAcquires.reserve(pendingAcquires.size() + handoffs.size());
            for (QueueOwnershipHandoffPayload& handoff : handoffs) {
                pendingAcquires.push_back(PendingAcquire{std::move(handoff), releaseSemaphore, releaseValue});
            }
        }

        //! Inverse of QueuePendingAcquires => take the handoffs addressed to this queue out of the
        //! global list. The context records them in RHIContext::FlushPendingAcquires
        [[nodiscard]] std::vector<PendingAcquire> TakePendingAcquires(EPoolQueueType queue) {
            std::lock_guard<std::mutex> lock(pendingAcquireMutex);
            if (pendingAcquires.empty()) { return {}; }

            //! satble partition so we just cut out the needed data from the back
            const auto mineBegin = std::stable_partition(pendingAcquires.begin(), pendingAcquires.end(),
                [queue](const PendingAcquire& pending) { return pending.handoff.dstQueue != queue; });

            std::vector<PendingAcquire> mine(std::make_move_iterator(mineBegin), std::make_move_iterator(pendingAcquires.end()));
            pendingAcquires.erase(mineBegin, pendingAcquires.end());
            return mine;
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
