import Lib.FrameData;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOutput mainVS(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);
    uint globalVtxID = vertexID + pc.firstInstance * 3; // for now
    float3 position = LoadPosition(frame.positionsRef, globalVtxID);
    output.position = mul(float4(position, 1.0f), frame.viewProj);

    output.uv = position.xy * 0.5f + 0.5f;
    //output.position = float4(position, 1.0f);
    return output;
}
