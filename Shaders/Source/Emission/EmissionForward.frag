#version 450

#extension GL_GOOGLE_include_directive : require

#include "../Base.glsl"

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNorm;

layout(location = 0) out vec4 outColor;

void main() {
    float camAngle = max(dot(-perFrame.camDirection.xyz, inWorldNorm), 0.05f);
    outColor = vec4(perObj.color.rgb * camAngle, 1.0f);
    //outColor = vec4(inWorldNorm * 0.5 + 0.5, 1.0f);
}