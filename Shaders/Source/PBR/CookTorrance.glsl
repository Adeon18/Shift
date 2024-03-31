#ifndef COOKTORRANCE_GLSL
#define COOKTORRANCE_GLSL

#include "../Lights.glsl"

#define PI 3.14159265f
#define MIN_DOT 0.001f


//! https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
// Converts a color from linear light gamma to sRGB gamma
vec3 FromLinear(vec3 linearRGB)
{
    bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));
    vec3 higher = vec3(1.055)*pow(linearRGB.rgb, vec3(1.0/2.4)) - vec3(0.055);
    vec3 lower = linearRGB.rgb * vec3(12.92);

    return mix(higher, lower, cutoff);
}

//! https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
// Converts a color from sRGB gamma to linear light gamma
vec3 ToLinear(vec3 sRGB)
{
    bvec3 cutoff = lessThan(sRGB.rgb, vec3(0.04045));
    vec3 higher = pow((sRGB.rgb + vec3(0.055))/vec3(1.055), vec3(2.4));
    vec3 lower = sRGB.rgb/vec3(12.92);

    return mix(higher, lower, cutoff);
}

// Schlick's approximation of Fresnel reflectance
vec3 Fresnel(float NoL, vec3 F0)
{
    return F0 + (1 - F0) * pow(1 - NoL, 5);
}

// GGX normal distribution,
// Real-Time Rendering 4th Edition, page 340, equation 9.41
// This code is taken from my other engine, LRNEngine https://github.com/Adeon18/LRNEngine
float NDF_GGX(float roughness, float NdotH)
{
    float roughnessSquared = roughness * roughness;

    float denom = NdotH * NdotH * (roughnessSquared - 1.0) + 1.0;
    denom = PI * denom * denom;
    return roughnessSquared / denom;
}

// Height-correlated Smith G2 for GGX,
// Filament, 4.4.2 Geometric shadowing
float GeometrySmith(float roughness, float NdotV, float NdotL)
{
    // HUH??? WHAT IS THIS???
    NdotV *= NdotV;
    NdotL *= NdotL;
    float roughnessSquared = roughness * roughness;
    return 2.0 / (sqrt(1 + roughnessSquared * (1 - NdotV) / NdotV) + sqrt(1 + roughnessSquared * (1 - NdotL) / NdotL));
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmithRemade(float roughness, float NdotV, float NdotL)
{
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 LambertDiffuse(vec3 albedo, vec3 norm, vec3 lightDir, vec3 F0, float metalness)
{
    float NdotL = max(dot(norm, lightDir), MIN_DOT);
    return (albedo * (1 - metalness) / PI) * (1 - Fresnel(NdotL, F0));
}

vec3 CookTorrenceSpecular(vec3 micNorm, vec3 halfVector, vec3 viewDir, vec3 lightDir, float roughness, vec3 F0)
{
    float NdotV = max(dot(micNorm, viewDir), MIN_DOT);
    float NdotL = max(dot(micNorm, lightDir), MIN_DOT);
    float HdotL = max(dot(halfVector, lightDir), MIN_DOT);
    float NdotH = max(dot(micNorm, halfVector), MIN_DOT);

    float N = NDF_GGX(roughness, NdotH);
    float G = GeometrySmithRemade(roughness, NdotV, NdotL);
    vec3 F = Fresnel(HdotL, F0);

    vec3 num = N * G * F;
    float den = 4.0 * NdotV * NdotL;

    return num / den;
}

vec3 CalculateDirectionalLightRadiance(DirectionalLight light, vec3 micNorm, vec3 viewDir, vec3 albedo, vec3 F0, float metallic, float roughness)
{
    vec3 lightDir = -light.direction.xyz;
    vec3 halfVector = normalize(viewDir + lightDir);

    float NdotL = max(dot(micNorm, lightDir), MIN_DOT);

    return light.radiance.rgb * NdotL *
    (LambertDiffuse(albedo, micNorm, lightDir, F0, metallic) +
    CookTorrenceSpecular(micNorm, halfVector, viewDir, lightDir, roughness, F0));
}

vec3 CalculatePointLightRadiance(PointLight light, vec3 micNorm, vec3 viewDir, vec3 worldPos, vec3 albedo, vec3 F0, float metallic, float roughness)
{
    vec3 lightDir = normalize(light.position.xyz - worldPos);
    vec3 halfVector = normalize(viewDir + lightDir);

    // Stupid in general, normal for point lights
    float distance = length(light.position.xyz - worldPos);
    float attenuation = 1.0 / (distance * distance);

    float NdotL = max(dot(micNorm, lightDir), MIN_DOT);

    return light.radiance.rgb * attenuation * NdotL *
    (LambertDiffuse(albedo, micNorm, lightDir, F0, metallic) +
     CookTorrenceSpecular(micNorm, halfVector, viewDir, lightDir, roughness, F0));
}


#endif // COOKTORRANCE_GLSL