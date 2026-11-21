struct SampleUniforms
{
    float4x4 transform;
    float4 color;
};

[[vk::binding(0, 0)]] ConstantBuffer<SampleUniforms> uniforms;

static const float2 POSITIONS[3] = { float2(-0.5, -0.4), float2(0.5, -0.4), float2(0.0, 0.6) };

float4 mainVS(uint vertexId : SV_VertexID) : SV_Position
{
    return mul(float4(POSITIONS[vertexId], 0.0, 1.0), uniforms.transform);
}

float4 mainPS() : SV_Target
{
    return uniforms.color;
}
