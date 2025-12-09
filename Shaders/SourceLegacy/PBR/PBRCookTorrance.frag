#version 450

#extension GL_GOOGLE_include_directive : require

#include "../Base.glsl"
#include "../Lights.glsl"

#include "CookTorrance.glsl"

layout(location = 0) in vec3 outWorldPos;
layout(location = 1) in vec3 outWorldNorm;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 1) uniform sampler2D TexDiffuse;
layout(set = 2, binding = 2) uniform sampler2D TexNormals;
layout(set = 2, binding = 3) uniform sampler2D TexMetallicRoughness;

void main() {
    vec4 colorTex = texture(TexDiffuse, fragTexCoord);
    vec3 albedo = colorTex.rgb;
    vec3 micNorm = texture(TexNormals, fragTexCoord).rgb;
    micNorm = micNorm * 2.0 - 1.0;
    micNorm = normalize(TBN * micNorm);
    micNorm = normalize(outWorldNorm + micNorm);
    //micNorm = outWorldNorm;
    vec3 MetRough = ToLinear(texture(TexMetallicRoughness, fragTexCoord).rgb);

    float metallic = clamp(MetRough.b, 0.05f, 0.99f);
    float roughness = clamp(MetRough.g, 0.05f, 0.99f);
    float occlusion = clamp(MetRough.r, 0.03f, 1.0f);
    //roughness *= roughness;
    //metallic *= metallic;

    vec3 viewDir = normalize(perFrame.camPosition.xyz - outWorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 outRadiance = 0.03f * occlusion * albedo.rgb;

    for (uint i = 0; i < lights.lightCounts.x; ++i) {
        outRadiance += CalculateDirectionalLightRadiance(lights.directionalLights[i], micNorm, viewDir, albedo, F0, metallic, roughness);
    }

    for (uint i = 0; i < lights.lightCounts.y; ++i) {
        outRadiance += CalculatePointLightRadiance(lights.pointLights[i], micNorm, viewDir, outWorldPos, albedo, F0, metallic, roughness);
    }

    outColor = vec4(vec3(outRadiance), colorTex.a);
}