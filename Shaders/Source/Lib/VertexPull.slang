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

//! Index pc.firstSubmeshInstance + SV_InstanceID
//! Must be masked with SUBMESH_INSTANCE_OBJECT_MASK to get either get the objectData index or get instance flags
uint LoadSubmeshInstanceRaw(FrameConstants frame, uint index)
{
    return ((uint*)frame.submeshInstanceBufferRef)[index];
}

uint GetObjectIndexFromSubmeshInstance(uint submeshInstance)
{
    return submeshInstance & SUBMESH_INSTANCE_OBJECT_MASK;
}

uint GetInstanceFlagsFromSubmeshInstance(uint submeshInstance)
{
    return submeshInstance & ~SUBMESH_INSTANCE_OBJECT_MASK;
}

//! One drawable instance. objectIndex comes out of the submesh instance list
ObjectData LoadObject(FrameConstants frame, uint objectIndex)
{
    ObjectData object = ((ObjectData*)frame.objectBufferRef)[objectIndex];
    return object;
}
