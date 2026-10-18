//! Light icons: one screen-aligned quad per light, drawn procedurally
import Lib.FrameData;
import Lib.LightFetch;

[[vk::push_constant]] LightIconPushConstants pc;

static const float2 QUAD_CORNERS[6] = {
    float2(-1.0f, -1.0f), float2(1.0f, -1.0f), float2(-1.0f, 1.0f),
    float2(-1.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f)
};

struct LightIconData
{
    float4 position : SV_Position;
    //! Where in the quad this pixel sits, -1..1
    float2 local : TEXCOORD0;
    float3 color : COLOR0;
    nointerpolation uint type : TEXCOORD1;
};

[shader("vertex")]
LightIconData mainVS(uint vertexID : SV_VertexID)
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);
    LightData light = LoadLight(frame, vertexID / 6);
    const float2 corner = QUAD_CORNERS[vertexID % 6];

    float4 clip = mul(float4(light.position, 1.0f), frame.viewProj);
    if (clip.w <= 0.0f)
    {
        //! VS discard
        clip = float4(0.0f, 0.0f, 2.0f, 1.0f);
    }
    else
    {
        //! mul by w so that the size increase is applied
        clip.xy += corner * pc.iconHalfSizeNDC * clip.w;
    }

    LightIconData output;
    output.position = clip;
    output.local = corner;
    output.color = light.color;
    output.type = (uint)light.type;
    return output;
}

static const float ICON_EDGE = 0.64f;

[shader("fragment")]
float4 mainPS(LightIconData input) : SV_Target
{
    const float radius = length(input.local);
    bool inside = radius < ICON_EDGE;

    //! One extra test per type, so the three read apart at a glance
    if (input.type == (uint)ELightType::Directional)
    {
        //! A sun: the disc plus a thin cross out to the quad edge
        inside = inside || (min(abs(input.local.x), abs(input.local.y)) < 0.10f && radius < 1.0f);
    }
    else if (input.type == (uint)ELightType::Spot)
    {
        //! A "triangle" below the disc
        inside = inside || (input.local.y < 0.0f && abs(input.local.x) < -input.local.y * 0.6f);
    }

    clip(inside ? 1.0f : -1.0f);

    //! Some silly rings around the icons
    float3 color = input.color;
    if (radius > 0.40f && radius <= 0.52f) { color = float3(1.0f, 1.0f, 1.0f); }
    else if (radius > 0.52f && radius <= ICON_EDGE) { color = float3(0.0f, 0.0f, 0.0f); }

    return float4(color, 1.0f);
}
