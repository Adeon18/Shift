#version 450

#extension GL_GOOGLE_include_directive : require

#include "../Base.glsl"
#include "../Lights.glsl"

layout(location = 0) in vec3 outWorldPos;
layout(location = 1) in vec3 outWorldNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);

    vec3 outRadiance = vec3(0.001f);

    for (uint i = 0; i < lights.lightCounts.x; ++i) {
        outRadiance += CalcDirectionalLightDiffuse(lights.directionalLights[i], outWorldNorm) * color.rgb;
    }

    for (uint i = 0; i < lights.lightCounts.y; ++i) {
        outRadiance += CalcPointLightDiffuse(lights.pointLights[i], outWorldNorm, outWorldPos) * color.rgb;
    }

    outColor = vec4(clamp(outRadiance, 0.0f, 1.0f), 1.0f);
}