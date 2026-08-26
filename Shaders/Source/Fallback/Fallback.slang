//! The shader used when a real one fails to compile.
//!
//! We kinda need to keep it similar to the main pipeline but this should be replaced later
struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldNormal : NORMAL0;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOutput mainVS(uint vertexID : SV_VertexID)
{
    //! I use vertex pulling now so no VB lol
    const float2 positions[3] = { float2(-0.5, -0.5), float2(0.5, -0.5), float2(0.0, 0.5) };

    VertexOutput output;
    output.position = float4(positions[vertexID % 3], 0.0, 1.0);
    output.worldNormal = float3(0.0, 1.0, 0.0);
    output.uv = float2(0.0, 0.0);

    return output;
}

[shader("fragment")]
float4 mainPS() : SV_Target
{
    return float4(0.75, 0.25, 0.75, 1.0);
}
