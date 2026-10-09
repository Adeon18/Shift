//! The editor selection box, one WS AABB per selection
import Lib.FrameData;

[[vk::push_constant]] SelectionBoxPushConstants pc;

struct SelectionBoxData
{
    float4 position : SV_Position;
};

[shader("vertex")]
SelectionBoxData mainVS(uint vertexID : SV_VertexID)
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);

    const float3 boxMin = pc.boxMinFraction.xyz;
    const float3 boxMax = pc.boxMaxPad.xyz;

    //! Segment - which line, endpoint - start/end
    const uint segment = vertexID / 2;
    const uint endpoint = vertexID % 2;
    //! Which corner and axis of 3 on it
    const uint corner = segment / 3;
    const uint axis = segment % 3;

    //! Hehe look at the trick I know
    const float3 cornerBits = float3((corner >> 0) & 1, (corner >> 1) & 1, (corner >> 2) & 1);
    const float3 cornerPos = lerp(boxMin, boxMax, cornerBits);

    //! Axis for corner part
    const float3 axisMask = float3(axis == 0 ? 1.0f : 0.0f, axis == 1 ? 1.0f : 0.0f, axis == 2 ? 1.0f : 0.0f);
    //! Whether we sitting on mix/max we will need to do +/- 1
    const float direction = (dot(cornerBits, axisMask) > 0.5f) ? -1.0f : 1.0f;
    const float step = float(endpoint) * direction * pc.boxMinFraction.w;

    const float3 position = cornerPos + axisMask * (boxMax - boxMin) * step;

    SelectionBoxData output;
    output.position = mul(float4(position, 1.0f), frame.viewProj);
    return output;
}

[shader("fragment")]
float4 mainPS(SelectionBoxData input) : SV_Target
{
    return pc.color;
}
