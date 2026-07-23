//
// Created by otrush on 7/5/2026.
//

#include "VKGPUProfiler.hpp"

#include "Graphics/RHI/Vulkan/VKDevice.hpp"
#include "Graphics/RHI/Vulkan/VKMacros.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::VK {

    void GPUProfiler::Init(const Device* device, uint32_t capacityRanges) {
        m_device = device;
        m_slotCapacity = capacityRanges * 2; //! begin + end timestamp per range
        m_tree.Init(capacityRanges);

        VkQueryPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = m_slotCapacity;

        m_pool = m_device->CreateQueryPool(info);
        if (!(VkNullCheck(m_pool))) {
            Log(Error, "Failed to create timestamp query pool");
            return;
        }

        //! CONSPECT: The number of ns the timestamp value takes to increment by 1
        m_timestampPeriodNs = m_device->GetDeviceProperties().limits.timestampPeriod;

        //! Mask off the invalid high bits of the graphics queue's timestamps (spec allows < 64)
        //! CONSPECT: The GPU does nto have perfect timespemp queries, we get what GPU has, sometimes it is 0, 36, 34, etc.
        //! bits. Its different per vendor and even per Engine in the GPU! We just get the timeline query value available.
        uint32_t validBits = 64;
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_device->GetPhysicalDevice(), &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_device->GetPhysicalDevice(), &familyCount, families.data());

        //! TODO: Probably different for timing diff queues
        const auto graphicsFamily = m_device->GetQueueFamilyIndices().graphicsFamily;
        if (graphicsFamily.has_value() && graphicsFamily.value() < familyCount) {
            const uint32_t queueValidBits = families[graphicsFamily.value()].timestampValidBits;
            if (queueValidBits > 0) {
                validBits = queueValidBits;
            }
        }
        m_validBitsMask = (validBits >= 64) ? ~0ull : ((1ull << validBits) - 1ull);
    }

    void GPUProfiler::Destroy() {
        if (m_pool != VK_NULL_HANDLE && m_device) {
            m_device->DestroyQueryPool(m_pool);
            m_pool = VK_NULL_HANDLE;
        }
    }

    void GPUProfiler::ResetForRecording(VkCommandBuffer cmd) {
        if (m_pool == VK_NULL_HANDLE) { return; }
        vkCmdResetQueryPool(cmd, m_pool, 0, m_slotCapacity);
        m_tree.Clear();
    }

    void GPUProfiler::PushRange(VkCommandBuffer cmd, const char* name) {
        const uint32_t slot = m_tree.OpenRange(name);
        if (slot != GPUTimeRangeTree::INVALID_SLOT) {
            WriteTimestamp(cmd, slot);
        }
    }

    void GPUProfiler::PopRange(VkCommandBuffer cmd) {
        const uint32_t slot = m_tree.CloseRange();
        if (slot != GPUTimeRangeTree::INVALID_SLOT) {
            WriteTimestamp(cmd, slot);
        }
    }

    std::vector<GPUTimeRange> GPUProfiler::Collect() {
        const uint32_t slotCount = m_tree.ActiveSlotCount();
        if (slotCount == 0) {
            return {};
        }
        std::vector<uint64_t> nanoseconds;
        ResolveNanoseconds(nanoseconds, slotCount);
        return m_tree.Resolve(nanoseconds);
    }

    void GPUProfiler::WriteTimestamp(VkCommandBuffer cmd, uint32_t slot) {
        if (m_pool == VK_NULL_HANDLE) { return; }
        //! CONSPECT: Essentially write the timestamp after all commands prior to it finished
        //! Can use other stuff apart from bottom of the pipe but this gives the clearest isolated pass timings.
        vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, m_pool, slot);
    }

    void GPUProfiler::ResolveNanoseconds(std::vector<uint64_t>& outNanoseconds, uint32_t slotCount) {
        outNanoseconds.assign(slotCount, 0);
        if (m_pool == VK_NULL_HANDLE || slotCount == 0) { return; }

        std::vector<uint64_t> raw(slotCount, 0);

        //! CONSPECT: WAIT_BIT is required even though the caller already waited on this slot's timeline:
        //! query availability is observable by the host only "some time after" the GPU finishes
        //! (Vulkan spec), so without it the driver may return VK_NOT_READY despite the work being
        //! done. The wait here does NOT stall as every slot in range was written by an
        //! already-completed frame (the tree only reports fully-written ranges).
        const VkResult res = vkGetQueryPoolResults(
            m_device->Get(), m_pool, 0, slotCount,
            raw.size() * sizeof(uint64_t), raw.data(), sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        if (res != VK_SUCCESS) {
            return;
        }

        for (uint32_t i = 0; i < slotCount; ++i) {
            const uint64_t ticks = raw[i] & m_validBitsMask;
            outNanoseconds[i] = static_cast<uint64_t>(static_cast<double>(ticks) * m_timestampPeriodNs);
        }
    }
} // Shift::VK
