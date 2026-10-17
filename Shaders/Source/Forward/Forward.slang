//! Forward Opaque pass
import Lib.FrameData;
import Lib.VertexPull;
import Lib.Bindless;
import Lib.MaterialFetch;
import Lib.BRDF;
import Lib.Lighting;
import Lib.Exposure;

[[vk::push_constant]] PushConstants pc;

struct ForwardData
{
    float4 position : SV_Position;
    float3 worldPos : TEXCOORD1;
    float3 worldNormal : NORMAL0;
    //! xyz = world tangent, w = the bitangent sign
    float4 worldTangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
ForwardData mainVS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);

    uint globalVertex = pc.meshVertexBase + vertexID;
    float3 position = LoadPosition(frame, globalVertex);
    float3 normal = LoadNormal(frame, globalVertex);
    float4 tangent = LoadTangent(frame, globalVertex);
    float2 uv = LoadUV(frame, globalVertex);

    const uint entry = LoadSubmeshInstanceRaw(frame, pc.firstSubmeshInstance + instanceID);
    ObjectData object = LoadObject(frame, GetObjectIndexFromSubmeshInstance(entry));

    //! For normal mapping, TODO, fully understand the math
    float3 m0 = object.model[0].xyz;
    float3 m1 = object.model[1].xyz;
    float3 m2 = object.model[2].xyz;
    float3x3 cofactor = float3x3(cross(m1, m2), cross(m2, m0), cross(m0, m1));

    float4 worldPos = mul(float4(position, 1.0f), object.model);

    ForwardData output;
    output.position = mul(worldPos, frame.viewProj);
    output.worldPos = worldPos.xyz;
    output.worldNormal = mul(normal, cofactor);
    output.worldTangent = float4(mul(tangent.xyz, float3x3(m0, m1, m2)), tangent.w);
    output.uv = uv;

    return output;
}

//! Small trick for better ambient
static const float AMBIENT_INTENSITY = 2000.0f;
static const float3 AMBIENT_SKY = float3(0.45f, 0.58f, 0.80f);
static const float3 AMBIENT_GROUND = float3(0.38f, 0.32f, 0.26f);

//! We can actually have one shader support discard without ruining earlyZ for the opaque shader via generics
interface IAlphaMode
{
    void Test(float alpha, float cutoff);
};

struct AlphaOpaque : IAlphaMode
{
    void Test(float alpha, float cutoff) {}
};

struct AlphaMasked : IAlphaMode
{
    void Test(float alpha, float cutoff) { clip(alpha - cutoff); }
};

float4 ShadeForward<TAlphaMode : IAlphaMode>(ForwardData input, bool isFrontFace)
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);
    MaterialData material = LoadMaterial(frame, pc.materialIndex);

    float4 baseColorSample = material.baseColorFactor * SampleTexture2D(material.baseColorTex, SAMPLER_ANISO_REPEAT, input.uv);

    TAlphaMode alphaMode;
    alphaMode.Test(baseColorSample.a, material.alphaCutoff);
    float3 baseColor = baseColorSample.rgb;

    //! Occlusion tex in gltf may differ or may be in the orm, this code handles both cases, either occlusionTex == ormTex,
    //! or they are different. No perf lost and 2 cases covered
    float occlusion = SampleTexture2D(material.occlusionTex, SAMPLER_ANISO_REPEAT, input.uv).r;
    float3 orm = SampleTexture2D(material.ormTex, SAMPLER_ANISO_REPEAT, input.uv).rgb;
    float perceptualRoughness = material.roughnessFactor * orm.g;
    float metallic = material.metallicFactor * orm.b;

    float3 emissive = material.emissiveFactor * SampleTexture2D(material.emissiveTex, SAMPLER_ANISO_REPEAT, input.uv).rgb;

    float3 geometricNormal = normalize(input.worldNormal);

    //! Gram-Smidt + normal mapping
    float3 T = input.worldTangent.xyz;
    T = normalize(T - geometricNormal * dot(geometricNormal, T));
    float3 B = cross(geometricNormal, T) * input.worldTangent.w;
    float3 tangentNormal = SampleTexture2D(material.normalTex, SAMPLER_ANISO_REPEAT, input.uv).rgb * 2.0f - 1.0f;
    float3 normal = normalize(tangentNormal.x * T + tangentNormal.y * B + tangentNormal.z * geometricNormal);

    //! 2 sided geo has reversed normals, reverse them back if we rasterize a face pointing in the same dir as camera
    if (!isFrontFace) { normal = -normal; }

    float3 viewDir = normalize(frame.cameraPosExposure.xyz - input.worldPos);
    SurfaceParams surface = MakeSurfaceParams(normal, viewDir, baseColor, metallic, perceptualRoughness);

    ActiveBRDF brdf;
    float3 color = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < frame.lightCount; ++i)
    {
        LightSample lightSample = SampleLight(frame, i, input.worldPos);
        float NoL = saturate(dot(surface.normal, lightSample.L));
        if (NoL <= 0.0f) { continue; }

        BRDFResult shaded = brdf.Evaluate(surface, lightSample.L);
        color += (shaded.diffuse + shaded.specular) * lightSample.irradiance * NoL;
    }

    float3 ambient = lerp(AMBIENT_GROUND, AMBIENT_SKY, surface.normal.y * 0.5f + 0.5f) * (AMBIENT_INTENSITY / PI);
    color += ambient * (surface.diffuseColor * occlusion + surface.f0);
    color += emissive;

    return float4(PreExpose(color, frame), 1.0f);
}

//! Opaque and masked shaders using simple generics
[shader("fragment")]
float4 mainPS(ForwardData input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    return ShadeForward<AlphaOpaque>(input, isFrontFace);
}

[shader("fragment")]
float4 mainPSMasked(ForwardData input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    return ShadeForward<AlphaMasked>(input, isFrontFace);
}
