struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

static const float2 POSITIONS[3] = { float2(-0.6, -0.6), float2(0.7, -0.4), float2(-0.1, 0.7) };
static const float3 COLORS[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };

VSOutput mainVS(uint vertexId : SV_VertexID)
{
    VSOutput output;
    output.position = float4(POSITIONS[vertexId], 0.0, 1.0);
    output.color = COLORS[vertexId];
    return output;
}

float4 mainPS(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
