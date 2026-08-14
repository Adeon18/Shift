//
// Created by otrush on 10/29/2025.
//

#ifndef SHIFT_RHI_VK_HPP
#define SHIFT_RHI_VK_HPP

//! Vulkan specializations of RenderHardwareInterface<RHI::Vulkan> members whose bodies must
//! touch native VK types (descriptor-set-layout construction, device wait-idle, present fence).
//!
//! This header is included at the END of RHI.hpp under SHIFT_VULKAN_BACKEND, so the full
//! RenderHardwareInterface template is already visible here. Keeping these bodies out of
//! RHI.hpp proper is the point of F-VKBLOCK: the agnostic RHI stays Vk*-free, and per
//! [temp.expl.spec]/6 the specializations are declared before any odr-use in every TU
//! (all consumers include RHI.hpp, which pulls this header before the use site).

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
    inline void RenderHardwareInterface<RHI::Vulkan>::WaitForImagePresent(uint32_t imageIndex) {
        m_presentFences[imageIndex]->Wait();
        m_presentFences[imageIndex]->Reset();
    }

} // Shift

#endif //SHIFT_RHI_VK_HPP
