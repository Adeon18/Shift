//! Here is the entire vertex pull logic from FrameData.
import Lib.FrameData;

float3 LoadPosition(FrameConstants frame, uint vertexIndex)
{
    return ((float3*)frame.positionsRef)[vertexIndex];
}

float3 LoadNormal(FrameConstants frame, uint vertexIndex)
{
    return ((float3*)frame.normalsRef)[vertexIndex];
}

//! bitangent = cross(normal, tangent.xyz) * tangent.w
float4 LoadTangent(FrameConstants frame, uint vertexIndex)
{
    return ((float4*)frame.tangentsRef)[vertexIndex];
}

float2 LoadUV(FrameConstants frame, uint vertexIndex)
{
    return ((float2*)frame.uvsRef)[vertexIndex];
}

//! One drawable instance. objectIndex is pc.objectIndex + SV_InstanceID
ObjectData LoadObject(FrameConstants frame, uint objectIndex)
{
    ObjectData object = ((ObjectData*)frame.objectBufferRef)[objectIndex];
    return object;
}
