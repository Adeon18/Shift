import Lib.FrameData;
import Lib.Bindless;

struct SurfaceInput
{
    float3 baseColor;
    float alpha;
    //! Normal map + back face correction
    float3 normal;
    float2 uv;
    float perceptualRoughness;
    float metallic;
    float occlusion;
    float3 emissive;
    //! Sampling-time, so only a producer that samples can fill it. Zero everywhere else
    float mipLevel;
};

SurfaceInput UnpackSurface(MaterialData material, float2 uv, float3 worldNormal, float4 worldTangent, bool isFrontFace)
{
    SurfaceInput surface;
    surface.uv = uv;
    surface.mipLevel = 0.0f;

    float4 baseColorSample = material.baseColorFactor * SampleTexture2D(material.baseColorTex, SAMPLER_ANISO_REPEAT, uv);
    surface.baseColor = baseColorSample.rgb;
    surface.alpha = baseColorSample.a;

    surface.occlusion = SampleTexture2D(material.occlusionTex, SAMPLER_ANISO_REPEAT, uv).r;
    float3 orm = SampleTexture2D(material.ormTex, SAMPLER_ANISO_REPEAT, uv).rgb;
    surface.perceptualRoughness = material.roughnessFactor * orm.g;
    surface.metallic = material.metallicFactor * orm.b;

    surface.emissive = material.emissiveFactor * SampleTexture2D(material.emissiveTex, SAMPLER_ANISO_REPEAT, uv).rgb;

    float3 geometricNormal = normalize(worldNormal);

    //! Gram-Smidt + normal mapping
    float3 T = worldTangent.xyz;
    T = normalize(T - geometricNormal * dot(geometricNormal, T));
    float3 B = cross(geometricNormal, T) * worldTangent.w;
    float3 tangentNormal = SampleTexture2D(material.normalTex, SAMPLER_ANISO_REPEAT, uv).rgb * 2.0f - 1.0f;
    float3 normal = normalize(tangentNormal.x * T + tangentNormal.y * B + tangentNormal.z * geometricNormal);

    //! Reverse notmal to keep 2 sided in check
    if (!isFrontFace) { normal = -normal; }
    surface.normal = normal;

    return surface;
}
