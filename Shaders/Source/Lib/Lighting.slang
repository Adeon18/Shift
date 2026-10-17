//! Direct lighting, separated from the brdf and other lighting data
import Lib.FrameData;
import Lib.LightFetch;

struct LightSample
{
    //! Normalized
    float3 L;
    //! Fallofs included
    float3 irradiance;
};

static const float MIN_ATTENUATION_DIST = 1e-4;

//! Karis method:
//! https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float DistanceWindow(float distSq, float range)
{
    float factor = distSq * (1.0f / (range * range));
    float finalFactor = max(1 - factor * factor, 0.0f);
    return finalFactor * finalFactor / max(distSq, MIN_ATTENUATION_DIST);
}

float ConeFalloff(float3 L, float3 spotDir, float2 cosAngles)
{
    float spotSize = max(cosAngles.x - cosAngles.y, MIN_ATTENUATION_DIST);
    float ang = dot(L, -spotDir);
    float attenuation = clamp(ang * (1.0f / spotSize) + (-cosAngles.y * (1.0f / spotSize)), 0.0f, 1.0f);
    return attenuation * attenuation;
}

LightSample SampleDirectional(LightData light, float3 worldPos)
{
    LightSample s;
    s.L = normalize(-light.direction);
    s.irradiance = light.intensity * light.color;
    return s;
}

LightSample SamplePoint(LightData light, float3 worldPos)
{
    LightSample s;
    float3 distVector = light.position - worldPos;
    float l = length(distVector);
    s.L = normalize(distVector);
    s.irradiance = light.intensity * light.color * DistanceWindow(l * l, light.range);
    return s;
}

LightSample SampleSpot(LightData light, float3 worldPos)
{
    LightSample s;
    float3 distVector = light.position - worldPos;
    float l = length(distVector);
    s.L = normalize(distVector);
    s.irradiance = light.intensity * light.color * DistanceWindow(l * l, light.range) * ConeFalloff(s.L, light.direction, light.spotAngles);
    return s;

}

//! Wawe-uniform for now
LightSample SampleLight(FrameConstants frame, uint index, float3 worldPos)
{
    LightData light = LoadLight(frame, index);
    switch (light.type)
    {
    case ELightType::Directional: return SampleDirectional(light, worldPos);
    case ELightType::Point:       return SamplePoint(light, worldPos);
    default:                      return SampleSpot(light, worldPos);
    }
}
