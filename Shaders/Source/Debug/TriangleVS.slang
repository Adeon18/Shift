struct VertexInput
{
    float3 pos : POSITION;
};

struct VertexOutput
{
    float4 position : SV_Position;
};

[shader("vertex")]
VertexOutput mainVS(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.pos.xyz, 1.0);

    return output;
}
