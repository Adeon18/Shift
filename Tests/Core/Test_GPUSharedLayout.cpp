//! CPU-only. The other half of the CPU/GPU data contract: Source/Graphics/Shared/GPUShared.h is
//! compiled by both C++ and Slang, and nothing in either language checks that the two agree.
//! With BDA there is no descriptor to validate against, so a mismatch is silent corruption -
//! the shader reads whatever bytes happen to sit at the offset it believes in (F-SHADERLAYOUT).
//!
//! WHY THIS FILE IS SHORT. C++ and Slang's scalar layout agree by CONSTRUCTION for every member
//! type used here: a float3 packs to 12 on both sides, a uint64_t aligns to 8 on both. So the
//! realistic ways the two can diverge are narrow - someone enables GLM's aligned gentypes, or a
//! Slang upgrade changes packing - and both of those move the struct's total SIZE. Asserting
//! sizes therefore catches the same failures a per-field offset table would, without a table of
//! measured numbers to re-measure every time a field is added or renamed. (There was one; it
//! broke a build the first time FrameConstants was restructured, and it caught nothing that the
//! sizes below would have missed.)
//!
//! These are static_asserts rather than runtime CHECKs on purpose: a layout drift fails the
//! BUILD, which is the one signal that cannot be skipped, skimmed past or left red.
//!
//! Sizes were MEASURED, not derived: the header was compiled with slangc 2025.22.1 using the
//! engine's own settings (scalar layout, row-major matrices, reached through a pointer) and the
//! ArrayStride decorations read out of the resulting SPIR-V. Re-measure rather than hand-compute
//! when a struct deliberately changes.
//!
//! P4.3 (F-SHADERLAYOUT, AC1) replaces the constants below with a query against Slang's live
//! reflection, at which point nothing here needs maintaining at all.
#include <doctest/doctest.h>

#include <cstddef>
#include <type_traits>

#include "Graphics/Shared/GPUShared.h"

using namespace Shift::GPU;

namespace {
    //! sizeof() must match the stride SPIR-V gives the struct, or an array of them walks off
    //! alignment on the very first element
    static_assert(sizeof(FrameConstants) == 296, "FrameConstants no longer matches its SPIR-V stride");
    static_assert(sizeof(ObjectData) == 160, "ObjectData no longer matches its SPIR-V stride");
    static_assert(sizeof(MaterialData) == 64, "MaterialData no longer matches its SPIR-V stride");
    static_assert(sizeof(LightData) == 64, "LightData no longer matches its SPIR-V stride");
    static_assert(sizeof(PushConstants) == 16, "the push block must stay at 16 bytes");

    //! offsetof (used below) is only well-defined on standard-layout types
    static_assert(std::is_standard_layout_v<FrameConstants>);
    static_assert(std::is_standard_layout_v<ObjectData>);
    static_assert(std::is_standard_layout_v<MaterialData>);
    static_assert(std::is_standard_layout_v<LightData>);
    static_assert(std::is_standard_layout_v<PushConstants>);

    //! A GPUBufferRef is the whole portability contract in one type: 64 bits, opaque, never
    //! dereferenced outside a Lib/ accessor
    static_assert(sizeof(GPUBufferRef) == 8, "a GPUBufferRef must stay 64 bits on every target");

    //! The one C++-side setting that would silently re-space every struct here without changing a
    //! single line of GPUShared.h
    static_assert(sizeof(float3) == 12, "glm vec3 is padded: aligned gentypes must stay off");
    static_assert(sizeof(float4x4) == 64, "glm mat4 is not 4 columns of 4 floats");

    //! The 8-byte members are the only ones whose C++ and scalar-layout alignment could be made to
    //! disagree by an unlucky field ordering, so they are worth checking by construction rather
    //! than by remembering. A ref landing off an 8-byte boundary means C++ inserted padding that
    //! the shader's view of the same declaration does not have
    static_assert(offsetof(FrameConstants, positionsRef)      % alignof(GPUBufferRef) == 0);
    static_assert(offsetof(FrameConstants, normalsRef)        % alignof(GPUBufferRef) == 0);
    static_assert(offsetof(FrameConstants, tangentsRef)       % alignof(GPUBufferRef) == 0);
    static_assert(offsetof(FrameConstants, uvsRef)            % alignof(GPUBufferRef) == 0);
    static_assert(offsetof(FrameConstants, objectBufferRef)   % alignof(GPUBufferRef) == 0);
    static_assert(offsetof(FrameConstants, materialBufferRef) % alignof(GPUBufferRef) == 0);
    static_assert(offsetof(FrameConstants, lightBufferRef)    % alignof(GPUBufferRef) == 0);
    static_assert(offsetof(PushConstants,  frameConstantsRef) % alignof(GPUBufferRef) == 0);
}

TEST_SUITE("GPUSharedLayout") {

//! Everything this file guards is a static_assert above, so reaching this case at all means the
//! contract held. The case exists so the guard is visible in the suite listing rather than being
//! a header that silently compiles
TEST_CASE("the shared CPU/GPU structs still match the layout the shaders were built against") {
    CHECK(sizeof(FrameConstants) == 296);
    CHECK(sizeof(ObjectData) == 160);
    CHECK(sizeof(MaterialData) == 64);
    CHECK(sizeof(LightData) == 64);
    CHECK(sizeof(PushConstants) == 16);
}

}
