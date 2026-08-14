//
// Created by otrush on 8/14/2026.
//

#include "BufferRangeAllocator.hpp"

#include <algorithm>

#include "Utility/Logging/LogMacros.hpp"

namespace Shift::Graphics {

    void BufferRangeAllocator::Init(uint32_t capacity) {
        m_capacity = capacity;
        m_freeBlocks.clear();
        if (capacity > 0) {
            m_freeBlocks.push_back({0, capacity});
        }
    }

    bool BufferRangeAllocator::Allocate(uint32_t count, Range *outRange) {
        if (count == 0 || !outRange) {
            return false;
        }

        //! The list is sorted, so the first block that fits is also the lowest one that fits
        for (auto freeBlock = m_freeBlocks.begin(); freeBlock != m_freeBlocks.end(); ++freeBlock) {
            if (freeBlock->count < count) { continue; }

            //! Carve off the FRONT: the remainder survives at the high end, which is where the
            //! next first-fit scan looks last
            *outRange = {freeBlock->first, count};
            freeBlock->first += count;
            freeBlock->count -= count;

            //! An exact fit erases the block - a zero-count one breaks the free-list invariant
            if (freeBlock->count == 0) {
                m_freeBlocks.erase(freeBlock);
            }
            return true;
        }

        return false;
    }

    void BufferRangeAllocator::Free(Range range) {
        if (range.count == 0) {
            return;
        }

        //! Insert sorted by first. Sorted is what reduces coalescing to a look at two neighbours
        const auto next = std::lower_bound(m_freeBlocks.begin(), m_freeBlocks.end(), range,
                                           [](const Range& a, const Range& b) { return a.first < b.first; });
        auto inserted = m_freeBlocks.insert(next, range);

        const auto after = inserted + 1;
        if (after != m_freeBlocks.end() && inserted->first + inserted->count == after->first) {
            inserted->count += after->count;
            m_freeBlocks.erase(after);
        }

        if (inserted != m_freeBlocks.begin()) {
            const auto before = inserted - 1;
            if (before->first + before->count == inserted->first) {
                before->count += inserted->count;
                m_freeBlocks.erase(inserted);
            }
        }
    }

    uint32_t BufferRangeAllocator::GetUsed() const {
        uint32_t free = 0;
        for (const Range& block : m_freeBlocks) {
            free += block.count;
        }
        return m_capacity - free;
    }

    uint32_t BufferRangeAllocator::GetLargestFreeRun() const {
        uint32_t largest = 0;
        for (const Range& block : m_freeBlocks) {
            if (block.count > largest) { largest = block.count; }
        }
        return largest;
    }

    bool BufferRangeAllocator::CheckInvariants() const {
        uint32_t previousEnd = 0;
        bool isFirst = true;

        for (const Range& block : m_freeBlocks) {
            if (block.count == 0) {
                LogError("BufferRangeAllocator: zero-count free block at {}", block.first);
                return false;
            }

            const uint64_t end = static_cast<uint64_t>(block.first) + block.count;
            if (end > m_capacity) {
                LogError("BufferRangeAllocator: free block [{}, {}) runs past capacity {}",
                         block.first, end, m_capacity);
                return false;
            }

            if (!isFirst) {
                if (block.first < previousEnd) {
                    LogError("BufferRangeAllocator: free block [{}, {}) overlaps the previous one, "
                             "which ended at {}", block.first, end, previousEnd);
                    return false;
                }
                //! Touching neighbours are not merely untidy: they mean a Free() inserted without
                //! coalescing, and the allocator will refuse allocations it has the room for
                if (block.first == previousEnd) {
                    LogError("BufferRangeAllocator: free blocks are adjacent at {} - a Free() did "
                             "not coalesce", block.first);
                    return false;
                }
            }

            previousEnd = static_cast<uint32_t>(end);
            isFirst = false;
        }

        return true;
    }
}
