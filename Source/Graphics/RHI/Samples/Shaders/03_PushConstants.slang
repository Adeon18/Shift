struct PushConstants
{
    float4 color;
    float2 offset;
    float scale;
    float _pad;
};

[[vk::push_constant]] PushConstants pc;

static const float2 POSITIONS[3] = { float2(-0.5, -0.4), float2(0.5, -0.4), float2(0.0, 0.5) };

float4 mainVS(uint vertexId : SV_VertexID) : SV_Position
{
    return float4(POSITIONS[vertexId] * pc.scale + pc.offset, 0.0, 1.0);
}

float4 mainPS() : SV_Target
{
    return pc.color;
}
