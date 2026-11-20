//
// Created by otrush on 10/29/2025.
//

#ifndef SHIFT_RHI_VK_HPP
#define SHIFT_RHI_VK_HPP

//! Vulkan specializations of RenderHardwareInterface<RHI::Vulkan> members whose bodies must
//! touch native VK types (descriptor-set-layout construction, device wait-idle, present fence).
//!
//! This header is included at the END of RHI.hpp under SHIFT_VULKAN_BACKEND

#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift {

    namespace RHIDetail {
        inline VkDescriptorSetLayout BuildSetLayout(VK::DescriptorLayoutCache& cache, const PipelineLayoutDescriptor& desc) {
            std::vector<VkDescriptorSetLayoutBinding> vkBindings;
            std::vector<VkDescriptorBindingFlags> bindingFlags;
            vkBindings.reserve(desc.bindings.size());
            bindingFlags.reserve(desc.bindings.size());

            bool anyUpdateAfterBind = false;

            for (const auto& b : desc.bindings) {
                VkDescriptorSetLayoutBinding binding{};
                binding.binding = b.binding;
                binding.descriptorCount = b.count;
                binding.stageFlags = VK::Util::ShiftToVKBindingVisibility(b.stageFlags);
                binding.descriptorType = VK::Util::ShiftToVKBindingType(b.type);
                binding.pImmutableSamplers = nullptr; // handle immutable samplers if needed
                vkBindings.push_back(binding);

                VkDescriptorBindingFlags flags = 0;
                if (b.isBindless) {
                    flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                }
                if (b.updateAfterBind) {
                    flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                             VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
                    anyUpdateAfterBind = true;
                }
                bindingFlags.push_back(flags);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
            layoutInfo.pBindings = vkBindings.data();

            VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
            flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            flagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
            flagsInfo.pBindingFlags = bindingFlags.data();
            layoutInfo.pNext = &flagsInfo;

            if (anyUpdateAfterBind) {
                layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            }

            return cache.CreateDescriptorLayout(layoutInfo, bindingFlags);
        }

        inline bool NeedsBindlessPool(const PipelineLayoutDescriptor& desc) {
            for (const auto& b : desc.bindings) {
                if (b.isBindless) { return true; }
            }
            return false;
        }
    } // RHIDetail

    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::WaitForGPU() {
        vkDeviceWaitIdle(m_local.device->Get());
    }

    template<>
    inline Pipeline* RenderHardwareInterface<RHI::Vulkan>::HandleCreator::CreatePipeline(const PipelineDescriptor &desc,
        const std::vector<ShaderStageDesc> &shaders)
    {

        std::vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(desc.descriptorLayouts.size());

        for (const auto& layoutDesc : desc.descriptorLayouts) {
            setLayouts.push_back(RHIDetail::BuildSetLayout(m_backend->m_local.descLayoutCache, layoutDesc));
        }

        return new Pipeline{m_backend->m_local.device.get(), desc, shaders, setLayouts};
    }

    template<>
    inline ResourceSet *RenderHardwareInterface<RHI::Vulkan>::HandleCreator::CreateResourceSet(const PipelineLayoutDescriptor &desc) {

        const VkDescriptorSetLayout layout = RHIDetail::BuildSetLayout(m_backend->m_local.descLayoutCache, desc);

        return new ResourceSet{
            m_backend->m_local.device.get(),
            m_backend->m_local.descAllocator->Allocate(layout, RHIDetail::NeedsBindlessPool(desc))
        };
    }

    template<>
    inline EAcquireResult RenderHardwareInterface<RHI::Vulkan>::AcquireFrameImage(FrameImage& out) {
        VK::BinarySemaphore* imageAvailable = m_local.imageAvailable[m_currentFrame].get();
        bool outOfDate = false;
        const uint32_t imageIdx = m_local.swapchain->AquireNextImage(*imageAvailable, &outOfDate);
        if (outOfDate) {
            return EAcquireResult::NeedsResize;
        }
        if (imageIdx == UINT32_MAX) {
            return EAcquireResult::Failed;
        }

        m_local.lastAcquireSemaphore = imageAvailable;
        //! This image's previous present must be done with its semaphores
        m_local.presentFences[imageIdx]->Wait();
        m_local.presentFences[imageIdx]->Reset();

        Texture& texture = m_local.swapchain->GetSwapchainTexture(imageIdx);
        //! Wait on color attachment output rather than last frame's bottom of the pipe - this caused me hella bugs
        ResourceStateTracker<RHI::Vulkan>::OnSwapchainAcquire(texture, VK::BinarySemaphore::WAIT_STAGE);
        out = FrameImage{&texture, imageIdx};
        return EAcquireResult::Ok;
    }

    template<>
    inline bool RenderHardwareInterface<RHI::Vulkan>::Present(const FrameImage& image, bool* needsResize) {
        return m_local.swapchain->Present(*m_local.renderFinished[image.index], image.index, needsResize, *m_local.presentFences[image.index]);
    }

} // Shift

#endif //SHIFT_RHI_VK_HPP
