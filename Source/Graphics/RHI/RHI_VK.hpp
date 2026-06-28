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

        // For each layout, create the descriptor set layout
        for (const auto& layoutDesc : desc.descriptorLayouts) {
            std::vector<VkDescriptorSetLayoutBinding> vkBindings;
            vkBindings.reserve(layoutDesc.bindings.size());

            for (const auto& b : layoutDesc.bindings) {
                VkDescriptorSetLayoutBinding binding{};
                binding.binding = b.binding;
                binding.descriptorCount = b.count;
                binding.stageFlags = VK::Util::ShiftToVKBindingVisibility(b.stageFlags);
                binding.descriptorType = VK::Util::ShiftToVKBindingType(b.type);
                binding.pImmutableSamplers = nullptr; // handle immutable samplers if needed
                vkBindings.push_back(binding);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
            layoutInfo.pBindings = vkBindings.data();

            setLayouts.push_back(m_backend->m_local.descLayoutCache.CreateDescriptorLayout(layoutInfo));
        }

        return new Pipeline{m_backend->m_local.device.get(), desc, shaders, setLayouts};
    }

    template<>
    inline ResourceSet *RenderHardwareInterface<RHI::Vulkan>::HandleCreator::CreateResourceSet(const PipelineLayoutDescriptor &desc) {

        //! TODO [CLEANUP] create a shared function for set pulling of set layout between this and pipeline creation
        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(desc.bindings.size());

        //! One binding-flags entry per binding: both bindless and regular and I have no damn clue whether this works
        std::vector<VkDescriptorBindingFlags> bindingFlags;
        bindingFlags.reserve(desc.bindings.size());

        bool containsBindless = false;
        //! We only can have one bindless structure in a single DS
        uint32_t bindlessCount = 0;
        EBindingType bindlessType = EBindingType::SampledImage;
        for (const auto& b : desc.bindings) {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = b.binding;
            binding.descriptorCount = b.count;
            binding.stageFlags = VK::Util::ShiftToVKBindingVisibility(b.stageFlags);
            binding.descriptorType = VK::Util::ShiftToVKBindingType(b.type);
            binding.pImmutableSamplers = nullptr; // handle immutable samplers if needed
            vkBindings.push_back(binding);

            if (b.isBindless) {
                bindingFlags.push_back(
                    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                    VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                    VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT);

                //! Only the first bindless binding drives the variable descriptor count
                if (!containsBindless) {
                    containsBindless = true;
                    bindlessCount = b.count;
                    bindlessType = b.type;
                }
            } else {
                bindingFlags.push_back(0);
            }
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
        layoutInfo.pBindings = vkBindings.data();

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
        flagsInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
        flagsInfo.pBindingFlags = bindingFlags.data();

        if (containsBindless) {
            layoutInfo.pNext = &flagsInfo;
            layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        }

        return new ResourceSet{m_backend->m_local.device.get(), m_backend->m_local.descAllocator->Allocate(m_backend->m_local.descLayoutCache.CreateDescriptorLayout(layoutInfo), bindlessCount, bindlessType)};
    }

    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::WaitForImagePresent(uint32_t imageIndex) {
        m_presentFences[imageIndex]->Wait();
        m_presentFences[imageIndex]->Reset();
    }

} // Shift

#endif //SHIFT_RHI_VK_HPP
