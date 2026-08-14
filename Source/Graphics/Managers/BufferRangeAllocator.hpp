//
// Created by otrush on 8/14/2026.
//

#ifndef SHIFT_BUFFERRANGEALLOCATOR_HPP
#define SHIFT_BUFFERRANGEALLOCATOR_HPP

#include <cstdint>
#include <vector>

namespace Shift::Graphics {

    //! An allocator without allocation, just tracks allocated ranges from a imaginary buffer
    //! The reasoning for it is to track pieces of allocation from multiple buffers of different objects size at once
    //! TODO: rename this shit as it is confusing? Idk I havce fooled myself with this already
    class BufferRangeAllocator {
    public:
        //! A half-open span of elements: [first, first + count)
        struct Range {
            uint32_t first = 0;
            uint32_t count = 0;

            bool operator==(const Range& o) const noexcept = default;
        };

        //! Reset to a single free block covering [0, capacity). Safe to call again to wipe the
        //! allocator as every Range handed out before is invalidated
        void Init(uint32_t capacity);

        //! The lowest free block that can hold an element wins
        [[nodiscard]] bool Allocate(uint32_t count, Range* outRange);

        //! Give a range back. Merges with the block before and/or after it when they touch, so
        //! freeing everything returns the allocator to exactly one block
        void Free(Range range);

        [[nodiscard]] uint32_t GetCapacity() const { return m_capacity; }

        //! Elements currently handed out
        [[nodiscard]] uint32_t GetUsed() const;

        [[nodiscard]] uint32_t GetFree() const { return m_capacity - GetUsed(); }

        //! Largest single allocation that could still succeed.
        [[nodiscard]] uint32_t GetLargestFreeRun() const;

        //! Number of free blocks. 1 means unfragmented
        [[nodiscard]] size_t GetFreeBlockCount() const { return m_freeBlocks.size(); }

        //! Debug function for checking whether we went out of the range
        [[nodiscard]] bool CheckInvariants() const;

    private:
        //! Sorted ascending by first, never overlapping, never adjacent, never zero-count.
        std::vector<Range> m_freeBlocks;
        uint32_t m_capacity = 0;
    };
}

#endif //SHIFT_BUFFERRANGEALLOCATOR_HPP
