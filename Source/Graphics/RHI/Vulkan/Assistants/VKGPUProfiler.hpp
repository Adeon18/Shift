//
// Created by otrush on 7/5/2026.
//

#ifndef SHIFT_VKGPUPROFILER_HPP
#define SHIFT_VKGPUPROFILER_HPP

#include <cstdint>
#include <vector>

#include "Utility/Vulkan/VKInclude.hpp"

#include "Graphics/RHI/Common/GPUProfiling.hpp"

namespace Shift::VK {
    class Device;

    //! Vulkan GPU-timing assistant. Owns the GPU tree with agnostic tree timing logic
    class GPUProfiler {
    public:
        GPUProfiler() = default;
        GPUProfiler(const GPUProfiler&) = delete;
        GPUProfiler& operator=(const GPUProfiler&) = delete;

        void Init(const Device* device, uint32_t capacityRanges);
        void Destroy();

        //! Cmd-reset the pool (must be outside a render pass) + drop the previous recording's ranges
        void ResetForRecording(VkCommandBuffer cmd);

        //! Open/close a nestable named range: allocates a tree slot and writes its timestamp.
        //! color tints the resolved range for the timing UI
        void PushRange(VkCommandBuffer cmd, const char* name, const DebugLabelColor& color = {});
        void PopRange(VkCommandBuffer cmd);

        //! Resolve the previous recording's ranges. Caller guarantees the GPU finished the buffer
        [[nodiscard]] std::vector<GPUTimeRange> Collect();

    private:
        void WriteTimestamp(VkCommandBuffer cmd, uint32_t slot);
        //! Read the first slotCount pool timestamps as absolute nanoseconds, so that the tree can handle the rest
        void ResolveNanoseconds(std::vector<uint64_t>& outNanoseconds, uint32_t slotCount);

        const Device* m_device = nullptr;
        VkQueryPool m_pool = VK_NULL_HANDLE;
        uint32_t m_slotCapacity = 0;
        float m_timestampPeriodNs = 1.0f;
        uint64_t m_validBitsMask = ~0ull;

        GPUTimeRangeTree m_tree;
    };
} // Shift::VK

#endif //SHIFT_VKGPUPROFILER_HPP
