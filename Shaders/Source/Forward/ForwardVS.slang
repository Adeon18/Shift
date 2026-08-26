import Lib.FrameData;
import Lib.VertexPull;

struct ForwardVSOutput
{
    float4 position : SV_Position;
    float3 worldNormal : NORMAL0;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
ForwardVSOutput mainVS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);
    ObjectData object = LoadObject(frame, pc.objectIndex + instanceID);

    float3 position = LoadPosition(frame, object.vertexOffset + vertexID);
    float3 normal = LoadNormal(frame, object.vertexOffset + vertexID);
    float2 uv = LoadUV(frame, object.vertexOffset + vertexID);

    ForwardVSOutput output;
    output.position = mul(mul(float4(position, 1.0f), object.model), frame.viewProj);
    output.worldNormal = mul(float4(normal, 0.0f), object.normalMat).xyz;
    output.uv = uv;

    return output;
}
