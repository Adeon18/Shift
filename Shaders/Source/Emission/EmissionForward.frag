#version 450

#extension GL_GOOGLE_include_directive : require

#include "../Base.glsl"

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNorm;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 cameraDir = normalize(perFrame.camPosition.xyz - inWorldPos);
    vec3 normedEmission = perObj.color.rgb / max(perObj.color.r,
                                                 max(perObj.color.g, max(perObj.color.b, 1.0)));
    float NoV = dot(cameraDir, inWorldNorm);
    //outColor = vec4(mix(normedEmission * 0.33, perObj.color.rgb, pow(max(0.0, NoV), 8.0)), 1.0f);
    outColor = vec4(perObj.color.rgb, 1.0f);
}