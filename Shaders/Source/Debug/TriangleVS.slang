import Lib.FrameData;

struct VertexOutput
{
    float4 position : SV_Position;
};

//! `pc`, the push-constant block, comes from Lib.FrameData - so no shader but that one spells a
//! backend-specific binding attribute

[shader("vertex")]
VertexOutput mainVS(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);
    uint globalVtxID = vertexID + pc.firstInstance * 3; // for now
    float3 position = LoadPosition(frame.positionsRef, globalVtxID);
    output.position = mul(float4(position, 1.0f), frame.viewProj);
    //output.position = float4(position, 1.0f);
    return output;
}
