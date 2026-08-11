//! Fixture for the scalar-buffer-layout gate. Not part of the engine's shader tree.
struct PositionStream
{
    float3 positions[4];
};

struct PushBlock
{
    //! The pull-model shape: an opaque address the shader indexes, not a bound vertex stream
    PositionStream* stream;
    uint index;
};

[[vk::push_constant]] PushBlock pc;

//! The other half of the comparison: a conventionally BOUND buffer, which is the shape
//! GLSL/std430 block-layout rules were written for
StructuredBuffer<PositionStream> boundStream;

[shader("vertex")]
float4 mainVS() : SV_Position
{
    return float4(pc.stream.positions[pc.index] + boundStream[0].positions[pc.index], 1.0);
}
