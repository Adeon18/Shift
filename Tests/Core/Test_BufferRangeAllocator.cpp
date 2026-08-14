#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

#include "Graphics/Managers/BufferRangeAllocator.hpp"

using Shift::Graphics::BufferRangeAllocator;
using Range = BufferRangeAllocator::Range;

//! These cases are the specification of Allocate/Free, in the order it is worth making
//! them pass: hand out space, hand it back, merge what is adjacent, refuse what does not fit.
//!
//! Every case that mutates the allocator ends by asserting CheckInvariants(), so a change that
//! produces the right ANSWER through a broken free list still fails here. That is deliberate:
//! the failure mode this class actually has is not "wrong range" but "free list slowly rots",
//! and by the time that shows up as a failed allocation the cause is hours behind you.

//! Ostap Note: This has to be like heaviluy tested as entire mesh data buffer will use this.

namespace {
    //! doctest asserts cannot contain && or || (hard static_assert), so compound conditions get
    //! hoisted into a named bool. This one comes up in every allocation check
    [[nodiscard]] bool IsRange(const Range& r, uint32_t first, uint32_t count) {
        return r.first == first && r.count == count;
    }
}

TEST_SUITE("BufferRangeAllocator") {

TEST_CASE("A fresh allocator is one free block covering everything") {
    BufferRangeAllocator alloc;
    alloc.Init(1000);

    CHECK(alloc.GetCapacity() == 1000);
    CHECK(alloc.GetUsed() == 0);
    CHECK(alloc.GetFree() == 1000);
    CHECK(alloc.GetLargestFreeRun() == 1000);
    CHECK(alloc.GetFreeBlockCount() == 1);
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("Allocations come out contiguous, in order, from the front") {
    BufferRangeAllocator alloc;
    alloc.Init(1000);

    Range a{}, b{}, c{};
    REQUIRE(alloc.Allocate(100, &a));
    REQUIRE(alloc.Allocate(250, &b));
    REQUIRE(alloc.Allocate(50, &c));

    CHECK(IsRange(a, 0, 100));
    CHECK(IsRange(b, 100, 250));
    CHECK(IsRange(c, 350, 50));

    CHECK(alloc.GetUsed() == 400);
    CHECK(alloc.GetFree() == 600);
    //! One allocation after another off the same block leaves a single remainder
    CHECK(alloc.GetFreeBlockCount() == 1);
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("Allocating the whole capacity works and leaves nothing behind") {
    BufferRangeAllocator alloc;
    alloc.Init(64);

    Range all{};
    REQUIRE(alloc.Allocate(64, &all));

    CHECK(IsRange(all, 0, 64));
    CHECK(alloc.GetUsed() == 64);
    //! An exact fit must ERASE the block, not leave a zero-count one
    CHECK(alloc.GetFreeBlockCount() == 0);
    CHECK(alloc.GetLargestFreeRun() == 0);
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("A request larger than any free run is refused without disturbing anything") {
    BufferRangeAllocator alloc;
    alloc.Init(100);

    Range a{};
    REQUIRE(alloc.Allocate(60, &a));

    Range tooBig{7, 7};
    CHECK_FALSE(alloc.Allocate(41, &tooBig));
    //! Refusal leaves the out-param untouched, so a caller that ignores the bool cannot silently
    //! act on a range that was never granted
    CHECK(IsRange(tooBig, 7, 7));
    CHECK(alloc.GetUsed() == 60);
    CHECK(alloc.CheckInvariants());

    //! ...and the run that DOES fit is still there
    Range fits{};
    REQUIRE(alloc.Allocate(40, &fits));
    CHECK(IsRange(fits, 60, 40));
}

TEST_CASE("A zero-count request is refused") {
    BufferRangeAllocator alloc;
    alloc.Init(10);

    Range r{3, 3};
    CHECK_FALSE(alloc.Allocate(0, &r));
    CHECK(IsRange(r, 3, 3));
    CHECK(alloc.GetUsed() == 0);
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("An exhausted allocator refuses everything, then recovers when space comes back") {
    BufferRangeAllocator alloc;
    alloc.Init(30);

    Range a{}, b{}, c{};
    REQUIRE(alloc.Allocate(10, &a));
    REQUIRE(alloc.Allocate(10, &b));
    REQUIRE(alloc.Allocate(10, &c));

    Range denied{};
    CHECK_FALSE(alloc.Allocate(1, &denied));
    CHECK(alloc.GetFree() == 0);

    alloc.Free(b);

    Range reused{};
    REQUIRE(alloc.Allocate(10, &reused));
    CHECK(IsRange(reused, 10, 10));
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("A freed range is handed out again") {
    BufferRangeAllocator alloc;
    alloc.Init(1000);

    Range a{}, b{};
    REQUIRE(alloc.Allocate(100, &a));
    REQUIRE(alloc.Allocate(100, &b));

    alloc.Free(a);
    CHECK(alloc.GetUsed() == 100);
    CHECK(alloc.CheckInvariants());

    //! First-fit means the low hole wins over the big tail
    Range reused{};
    REQUIRE(alloc.Allocate(100, &reused));
    CHECK(IsRange(reused, 0, 100));
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("First-fit takes the lowest hole that fits, not the first hole it sees") {
    BufferRangeAllocator alloc;
    alloc.Init(1000);

    Range a{}, b{}, c{}, d{};
    REQUIRE(alloc.Allocate(10, &a));   // [0, 10)
    REQUIRE(alloc.Allocate(50, &b));   // [10, 60)
    REQUIRE(alloc.Allocate(10, &c));   // [60, 70)
    REQUIRE(alloc.Allocate(50, &d));   // [70, 120)

    alloc.Free(a);                     // 10-wide hole at 0
    alloc.Free(c);                     // 10-wide hole at 60
    REQUIRE(alloc.GetFreeBlockCount() == 3);

    //! 30 does not fit the hole at 0, nor the one at 60 - it must skip both and take the tail
    Range big{};
    REQUIRE(alloc.Allocate(30, &big));
    CHECK(IsRange(big, 120, 30));

    //! ...while a 10 takes the LOWEST hole
    Range small{};
    REQUIRE(alloc.Allocate(10, &small));
    CHECK(IsRange(small, 0, 10));
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("Freeing a range adjacent to a hole merges the two") {
    BufferRangeAllocator alloc;
    alloc.Init(300);

    Range a{}, b{}, c{};
    REQUIRE(alloc.Allocate(100, &a));  // [0, 100)
    REQUIRE(alloc.Allocate(100, &b));  // [100, 200)
    REQUIRE(alloc.Allocate(100, &c));  // [200, 300)

    SUBCASE("merging forwards, with the block after it") {
        alloc.Free(c);                 // free tail is [200, 300)
        alloc.Free(b);                 // must extend it down to [100, 300)

        CHECK(alloc.GetFreeBlockCount() == 1);
        CHECK(alloc.GetLargestFreeRun() == 200);
    }

    SUBCASE("merging backwards, with the block before it") {
        alloc.Free(a);                 // free head is [0, 100)
        alloc.Free(b);                 // must extend it up to [0, 200)

        CHECK(alloc.GetFreeBlockCount() == 1);
        CHECK(alloc.GetLargestFreeRun() == 200);
    }

    SUBCASE("merging both ways at once - the case that catches a one-sided merge") {
        alloc.Free(a);
        alloc.Free(c);
        REQUIRE(alloc.GetFreeBlockCount() == 2);

        alloc.Free(b);                 // the middle closes the gap between both neighbours

        CHECK(alloc.GetFreeBlockCount() == 1);
        CHECK(alloc.GetLargestFreeRun() == 300);
    }

    CHECK(alloc.CheckInvariants());
}

TEST_CASE("A non-adjacent free stays its own block") {
    BufferRangeAllocator alloc;
    alloc.Init(300);

    Range a{}, b{}, c{};
    REQUIRE(alloc.Allocate(100, &a));
    REQUIRE(alloc.Allocate(100, &b));
    REQUIRE(alloc.Allocate(100, &c));

    alloc.Free(a);
    alloc.Free(c);

    //! Nothing touches, so nothing merges - two holes with b still live between them
    CHECK(alloc.GetFreeBlockCount() == 2);
    CHECK(alloc.GetUsed() == 100);
    CHECK(alloc.GetLargestFreeRun() == 100);
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("Freeing a zero-count range does nothing") {
    BufferRangeAllocator alloc;
    alloc.Init(100);

    Range a{};
    REQUIRE(alloc.Allocate(40, &a));

    alloc.Free({12, 0});

    CHECK(alloc.GetUsed() == 40);
    CHECK(alloc.GetFreeBlockCount() == 1);
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("Freeing everything, in any order, restores the allocator exactly") {
    BufferRangeAllocator alloc;
    alloc.Init(1024);

    std::vector<Range> ranges;
    const uint32_t sizes[] = {7, 200, 1, 64, 300, 13, 128, 5};
    for (uint32_t size : sizes) {
        Range r{};
        REQUIRE(alloc.Allocate(size, &r));
        ranges.push_back(r);
    }
    REQUIRE(alloc.CheckInvariants());

    //! Out of order on purpose: coalescing must not depend on freeing in allocation order
    const size_t order[] = {3, 0, 6, 1, 7, 4, 2, 5};
    for (size_t idx : order) {
        alloc.Free(ranges[idx]);
        REQUIRE(alloc.CheckInvariants());
    }

    //! Back to a virgin allocator: one block, whole capacity, no leaked fragments
    CHECK(alloc.GetUsed() == 0);
    CHECK(alloc.GetFreeBlockCount() == 1);
    CHECK(alloc.GetLargestFreeRun() == 1024);

    //! ...and it can hand the whole thing out again
    Range all{};
    REQUIRE(alloc.Allocate(1024, &all));
    CHECK(IsRange(all, 0, 1024));
}

TEST_CASE("Init resets a used allocator") {
    BufferRangeAllocator alloc;
    alloc.Init(100);

    Range a{};
    REQUIRE(alloc.Allocate(60, &a));

    alloc.Init(500);

    CHECK(alloc.GetCapacity() == 500);
    CHECK(alloc.GetUsed() == 0);
    CHECK(alloc.GetFreeBlockCount() == 1);
    CHECK(alloc.CheckInvariants());
}

TEST_CASE("A zero-capacity allocator refuses everything without tripping an invariant") {
    BufferRangeAllocator alloc;
    alloc.Init(0);

    Range r{};
    CHECK_FALSE(alloc.Allocate(1, &r));
    CHECK(alloc.GetFreeBlockCount() == 0);
    CHECK(alloc.GetUsed() == 0);
    CHECK(alloc.CheckInvariants());
}

//! The four SoA vertex streams share ONE allocator instance, so this is not really a separate
//! feature - it is the reason the class counts elements. Pinned here because "positions and uvs
//! disagree about where a mesh starts" is invisible until it renders as garbage
TEST_CASE("One allocation addresses every SoA stream, because it is measured in elements") {
    BufferRangeAllocator vertices;
    vertices.Init(2000);

    Range meshA{}, meshB{};
    REQUIRE(vertices.Allocate(1500, &meshA));
    REQUIRE(vertices.Allocate(400, &meshB));

    //! The same [first, count) indexes positions (12 B), normals (12 B), tangents (16 B) and
    //! uvs (8 B). Byte offsets differ per stream; the element index does not
    CHECK(IsRange(meshA, 0, 1500));
    CHECK(IsRange(meshB, 1500, 400));
    CHECK(meshB.first == meshA.first + meshA.count);
    CHECK(vertices.GetFree() == 100);
}

} // TEST_SUITE
