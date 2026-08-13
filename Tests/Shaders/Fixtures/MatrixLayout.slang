//! Fixture for the matrix-storage gate. Not part of the engine's shader tree.
//! A float4x4 reached two ways: through a pointer (how the engine reaches FrameConstants) and
//! through a bound buffer (which carries the explicit SPIR-V layout decoration).
struct Transforms
{
    float4x4 model;
};

struct PushBlock
{
    Transforms* transforms;
    uint index;
};

[[vk::push_constant]] PushBlock pc;

StructuredBuffer<Transforms> boundTransforms;

[shader("vertex")]
float4 mainVS() : SV_Position
{
    //! mul(v, M) is the row-vector convention: paired with row-major storage it computes glm's M * v
    return mul(float4(1.0, 0.0, 0.0, 1.0), pc.transforms[pc.index].model) +
           mul(float4(0.0, 1.0, 0.0, 1.0), boundTransforms[0].model);
}
