//! CPU-only. The per-frame ring's slot arithmetic is the one part of it that can be wrong with no
//! GPU in sight: get the stride wrong and frame N's write lands inside frame N-1's slot, which
//! looks like a synchronization bug (torn matrices, one frame behind) rather than the arithmetic
//! bug it is. Everything else about the ring - mapping, addressing, the write-after-BeginFrame
//! rule - needs a live device and is covered by the [app] smoke.
#include <doctest/doctest.h>

#include <cstdint>

#include "Config/EngineConfig.hpp"
#include "Graphics/Managers/FrameRingBuffer.hpp"
#include "Graphics/Shared/GPUShared.h"

using Shift::Graphics::FRAME_RING_SLOT_ALIGNMENT;
using Shift::Graphics::FrameRingSlotStride;

TEST_SUITE("FrameRingBuffer") {

TEST_CASE("slot stride rounds up to the slot alignment") {
    //! Exact multiples must not grow: a stride that over-rounds wastes a whole slot per frame
    CHECK(FrameRingSlotStride(FRAME_RING_SLOT_ALIGNMENT) == FRAME_RING_SLOT_ALIGNMENT);
    CHECK(FrameRingSlotStride(2 * FRAME_RING_SLOT_ALIGNMENT) == 2 * FRAME_RING_SLOT_ALIGNMENT);

    //! Anything else rounds up to the next multiple
    CHECK(FrameRingSlotStride(1) == FRAME_RING_SLOT_ALIGNMENT);
    CHECK(FrameRingSlotStride(FRAME_RING_SLOT_ALIGNMENT - 1) == FRAME_RING_SLOT_ALIGNMENT);
    CHECK(FrameRingSlotStride(FRAME_RING_SLOT_ALIGNMENT + 1) == 2 * FRAME_RING_SLOT_ALIGNMENT);

    //! Degenerate but well-defined: no bytes needs no space
    CHECK(FrameRingSlotStride(0) == 0);
}

//! Rounding bugs live at the boundaries, so the interesting inputs are the ones straddling a
//! multiple - not every byte count in a range. (This was a 1024-iteration sweep, which passed
//! just as well while contributing ~3000 of the suite's assertions and drowning the headline
//! number for every other test.)
TEST_CASE("slot stride holds its invariants at the boundaries") {
    constexpr uint64_t A = FRAME_RING_SLOT_ALIGNMENT;
    const uint64_t interesting[] = { 0, 1, A - 1, A, A + 1, 2 * A - 1, 2 * A, 2 * A + 1 };

    for (const uint64_t bytes : interesting) {
        const uint64_t stride = FrameRingSlotStride(bytes);
        CAPTURE(bytes);
        CAPTURE(stride);

        //! It must fit the payload...
        CHECK(stride >= bytes);
        //! ...and every slot boundary must stay aligned, since the slot address is handed to a
        //! shader as a GPUBufferRef and dereferenced there
        CHECK(stride % FRAME_RING_SLOT_ALIGNMENT == 0);
        //! ...without wasting a whole extra slot
        CHECK(stride - bytes < FRAME_RING_SLOT_ALIGNMENT);
    }
}

//!  one struct per frame, and a big array per frame
TEST_CASE("the real payloads slot without waste") {
    //! FrameConstants is 280 bytes, so it costs two 256-byte slots
    CHECK(FrameRingSlotStride(sizeof(Shift::GPU::FrameConstants)) == 512);

    //! The P4.2 ObjectData array is a whole number of slots already: 16384 * 160 bytes
    constexpr uint64_t objectArrayBytes = 16384ull * sizeof(Shift::GPU::ObjectData);
    CHECK(FrameRingSlotStride(objectArrayBytes) == objectArrayBytes);

    //! And the whole allocation is one slot per frame in flight, never more
    const uint64_t totalBytes =
            FrameRingSlotStride(sizeof(Shift::GPU::FrameConstants)) * Shift::Conf::SHIFT_MAX_FRAMES_IN_FLIGHT;
    CHECK(totalBytes == 512 * Shift::Conf::SHIFT_MAX_FRAMES_IN_FLIGHT);
}

}
