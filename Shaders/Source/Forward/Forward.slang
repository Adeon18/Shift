//! Forward Opaque pass
import Lib.FrameData;
import Lib.VertexPull;
import Lib.Bindless;
import Lib.MaterialFetch;
import Lib.BRDF;

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

    ObjectData object = LoadObject(frame, pc.objectIndex + instanceID);

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

//! One hardcoded directional light
static const float3 LIGHT_DIRECTION = float3(-0.4f, -1.0f, -0.35f);
static const float3 LIGHT_COLOR = float3(1.0f, 0.9f, 0.95f);
//! So it ooks better ons screen rn
static const float LIGHT_INTENSITY = PI;
static const float AMBIENT = 0.03f;

[shader("fragment")]
float4 mainPS(ForwardData input) : SV_Target
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);
    MaterialData material = LoadMaterial(frame, pc.materialIndex);

    float3 baseColor = material.baseColorFactor.rgb * SampleTexture2D(material.baseColorTex, SAMPLER_ANISO_REPEAT, input.uv).rgb;

    //! glTF packs occlusion in R, roughness in G and metallic in B
    float3 orm = SampleTexture2D(material.ormTex, SAMPLER_ANISO_REPEAT, input.uv).rgb;
    float occlusion = orm.r;
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

    float3 viewDir = normalize(frame.cameraPosExposure.xyz - input.worldPos);
    SurfaceParams surface = MakeSurfaceParams(normal, viewDir, baseColor, metallic, perceptualRoughness);

    float3 lightDir = -normalize(LIGHT_DIRECTION);
    float NoL = saturate(dot(surface.normal, lightDir));

    ActiveBRDF brdf;
    BRDFResult direct = brdf.Evaluate(surface, lightDir);

    float3 color = (direct.diffuse + direct.specular) * NoL * LIGHT_COLOR * LIGHT_INTENSITY;

    color += AMBIENT * (surface.diffuseColor * occlusion + surface.f0);
    color += emissive;

    return float4(color, 1.0f);
}
