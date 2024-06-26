#version 450

#extension GL_GOOGLE_include_directive : require

#include "ToneMappingOperators.glsl"

void main() {
    vec3 HDR = texture(texSampler, texCoords).rgb;
    HDR = AdjustExposure(HDR, UBO.data.x, UBO.data.y);
    vec3 LDR = ReinhardToneMapping(HDR);

    outColor = vec4(vec3(LDR), 1.0);
}