import Lib.FrameData;
import Lib.Bindless;
import Lib.MaterialFetch;

struct ForwardVSOutput
{
    float4 position : SV_Position;
    float3 worldNormal : NORMAL0;
    float2 uv : TEXCOORD0;
};

static const float3 LIGHT_DIRECTION = float3(-0.4f, -1.0f, -0.35f);
static const float3 LIGHT_COLOR = float3(1.0f, 0.9f, 0.95f);
static const float AMBIENT = 0.03f;

[shader("fragment")]
float4 mainPS(ForwardVSOutput input) : SV_Target
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);
    MaterialData material = LoadMaterial(frame, pc.materialIndex);
    float3 normal = normalize(input.worldNormal);
    float NoL = saturate(dot(normal, -normalize(LIGHT_DIRECTION)));

    float3 baseColor = SampleTexture2D(material.baseColorTex, SAMPLER_ANISO_REPEAT, input.uv).rgb;

    return float4(material.baseColorFactor.rgb * baseColor * (NoL * LIGHT_COLOR + AMBIENT), 1.0f);
}
