#version 450

#extension GL_GOOGLE_include_directive : require

#include "ToneMappingOperators.glsl"

void main() {
    vec3 HDR = texture(texSampler, texCoords).rgb;
    vec3 LDR = LumaBasedReinhardToneMapping(HDR);

    LDR = GammaCorrect(LDR, GAMMA);

    outColor = vec4(LDR, 1.0);
}