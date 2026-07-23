//
// Created by otrush on 7/5/2026.
//

#ifndef SHIFT_GPUPROFILING_HPP
#define SHIFT_GPUPROFILING_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "Graphics/RHI/Common/Base.hpp"

namespace Shift {
    //! One resolved GPU timing range. Backend-agnostic so the same result type is
    //! exposed regardless of the RHI backend. Results lag by SHIFT_MAX_FRAMES_IN_FLIGHT frames
    struct GPUTimeRange {
        std::string name;
        //! Tint for the timing UI. Only filled when debug range exists
        DebugLabelColor color{};
        uint32_t depth = 0;
        float milliseconds = 0.0f;
    };

    //! Backend-agnostic  system for assembling the timing tree
    class GPUTimeRangeTree {
    public:
        static constexpr uint32_t INVALID_SLOT = UINT32_MAX;

        //! capacityRanges = how many ranges fit; the backend pool must hold capacityRanges*2 slots
        void Init(uint32_t capacityRanges) { m_capacityRanges = capacityRanges; }

        void Clear() {
            m_records.clear();
            m_openStack.clear();
        }

        //! Open a range. Returns the slot to write the begin timestamp to, or INVALID_SLOT when the
        //! per-frame budget is exhausted
        uint32_t OpenRange(const char* name, const DebugLabelColor& color = {}) {
            if (m_records.size() >= m_capacityRanges) {
                m_openStack.push_back(INVALID_SLOT);
                return INVALID_SLOT;
            }
            const uint32_t rangeIdx = static_cast<uint32_t>(m_records.size());
            m_records.push_back(Record{
                .name = name ? name : "Unnamed",
                .color = color,
                .depth = static_cast<uint32_t>(m_openStack.size()),
                .beginSlot = rangeIdx * 2,
            });
            m_openStack.push_back(rangeIdx);
            return rangeIdx * 2;
        }

        //! Close the innermost open range. Returns the slot to write the end timestamp to, or
        //! INVALID_SLOT for an unbalanced close or an over-budget range
        uint32_t CloseRange() {
            if (m_openStack.empty()) { return INVALID_SLOT; }
            const uint32_t rangeIdx = m_openStack.back();
            m_openStack.pop_back();
            if (rangeIdx == INVALID_SLOT) { return INVALID_SLOT; }
            return m_records[rangeIdx].beginSlot + 1;
        }

        [[nodiscard]] uint32_t ActiveSlotCount() const { return static_cast<uint32_t>(m_records.size()) * 2; }

        //! Assemble the results from resolved absolute nanoseconds per slot (from the backend pool)
        [[nodiscard]] std::vector<GPUTimeRange> Resolve(std::span<const uint64_t> slotNanoseconds) const {
            std::vector<GPUTimeRange> out;
            out.reserve(m_records.size());
            for (const Record& r : m_records) {
                if (r.beginSlot + 1 >= slotNanoseconds.size()) { break; } // no data for this
                const uint64_t begin = slotNanoseconds[r.beginSlot];
                const uint64_t end = slotNanoseconds[r.beginSlot + 1];
                const float ms = (end >= begin) ? static_cast<float>(end - begin) * 1.0e-6f : 0.0f;
                out.push_back(GPUTimeRange{r.name, r.color, r.depth, ms});
            }
            return out;
        }

    private:
        struct Record {
            std::string name;
            DebugLabelColor color;
            uint32_t depth = 0;
            uint32_t beginSlot = 0; //! end slot is beginSlot + 1
        };

        std::vector<Record> m_records;      //! pre-order, cleared each recording
        std::vector<uint32_t> m_openStack;  //! indices into m_records (or INVALID_SLOT )
        uint32_t m_capacityRanges = 0;
    };
} // Shift

#endif //SHIFT_GPUPROFILING_HPP
