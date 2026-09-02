//
// Created by otrush on 8/11/2026.
//
//! The CPU/GPU data contract. This file is compiled by both C++ and Slang
//!
//! Rules for editing:
//!  - C++ types come in through the setup below. Write shader types (float3, uint) in the structs.
//!  - Keep 8-byte members (GPUBufferRef) at 8-byte-aligned offsets and pad explicitly. Both
//!    languages then agree without either one inserting padding of its own.
//!  - Only Slang modules under Shaders/Source/Lib/ may #include this file; entry points import
//!    those modules instead

#ifndef SHIFT_GPUSHARED_H
#define SHIFT_GPUSHARED_H

#ifdef __cplusplus
#include <cstdint>

#include <glm/glm.hpp>

namespace Shift::GPU {
    using float2 = glm::vec2;
    using float3 = glm::vec3;
    using float4 = glm::vec4;
    using float4x4 = glm::mat4;
    using uint = uint32_t;

    using GPUBufferRef = uint64_t;
#else
typedef uint64_t GPUBufferRef;
#endif

//! Slots of the global sampler array (set 0, binding 0) that shader code names at compile time.
static const uint SAMPLER_LINEAR_REPEAT  = 0;
static const uint SAMPLER_LINEAR_CLAMP   = 1;
static const uint SAMPLER_NEAREST_REPEAT = 2;
static const uint SAMPLER_ANISO_REPEAT   = 3;

//! Per-frame, per-view constants. Written into this frame's ring slot on the CPU and reached by
//! the shader through the address in PushConstants
struct FrameConstants {
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;

    float4 cameraPosExposure;
    float4 cameraDir; //! w-unused

    uint lightCount;
    //! UINT32_MAX means "nothing selected"
    uint selectedObject;
    uint debugViewMode;
    uint _pad0;

    //! The SoA vertex streams. All are indexed by the same global vertex index
    GPUBufferRef positionsRef;
    GPUBufferRef normalsRef;
    GPUBufferRef tangentsRef;
    GPUBufferRef uvsRef;

    //! The per-frame scene arrays
    GPUBufferRef objectBufferRef;
    GPUBufferRef materialBufferRef;
    GPUBufferRef lightBufferRef;
};

struct ObjectData {
    float4x4 model;
    //! Full inverse-transpose for now lolol
    float4x4 normalMat;
    //! xyz = center, w = radius.
    float4 boundsSphere;
};

//! A material:D
struct MaterialData {
    float4 baseColorFactor;

    float3 emissiveFactor;
    float alphaCutoff;

    //! Bindless slot indices into the global sampled-image array.
    uint baseColorTex;
    uint normalTex;
    uint ormTex;
    uint emissiveTex;

    float metallicFactor;
    float roughnessFactor;
    //! bit0 = alpha test
    uint flags;
    uint _pad0;
};

//! One light of any type. 64 bytes
struct LightData {
    //! TODO: just a thought, dir/point could use pos OR dir but it will break at spot lightis
    float3 position;
    //! 0 = directional, 1 = point, 2 = spot
    uint type;

    float3 direction;
    float range;

    float3 color;
    float intensity;

    //! x = inner cosine, y = outer cosine
    float2 spotAngles;
    float _pad0;
    float _pad1;
};

//! The ONLY push-constant block in the engine, 24B
struct PushConstants {
    GPUBufferRef frameConstantsRef;
    //! API agnostic firstInstance
    uint objectIndex;
    uint materialIndex;
    //! This mesh's base vertex in the merged streams
    uint meshVertexBase;
    uint _pad0;
};

#ifdef __cplusplus
} // namespace Shift::GPU
#endif

#endif // SHIFT_GPUSHARED_H
