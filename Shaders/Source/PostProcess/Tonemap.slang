//! Tonemap pass
import Lib.FrameData;
import Lib.Bindless;

[[vk::push_constant]] TonemapPushConstants pc;

struct TonemapData
{
    float4 position : SV_Position;
};

[shader("vertex")]
TonemapData mainVS(uint vertexID : SV_VertexID)
{
    float2 corner = float2(float((vertexID << 1) & 2), float(vertexID & 2));

    TonemapData output;
    output.position = float4(corner * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 mainPS(TonemapData input) : SV_Target
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);

    float3 hdr = LoadTexture2D(pc.hdrColorTex, int2(input.position.xy)).rgb;
    float3 exposed = hdr * frame.cameraPosExposure.w;

    return float4(saturate(exposed), 1.0f);
}
