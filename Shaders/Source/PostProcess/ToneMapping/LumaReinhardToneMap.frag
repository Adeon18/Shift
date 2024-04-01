#version 450

#extension GL_GOOGLE_include_directive : require

#include "ToneMappingOperators.glsl"

void main() {
    vec3 HDR = texture(texSampler, texCoords).rgb;
    HDR = AdjustExposure(HDR, UBO.data.x, UBO.data.y);
    vec3 LDR = LumaBasedReinhardToneMapping(HDR);

    LDR = GammaCorrect(LDR, GAMMA, UBO.data.z);

    outColor = vec4(LDR, 1.0);
}