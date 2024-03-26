#version 450

#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

#define GAMMA 2.2

vec3 lumaBasedReinhardToneMapping(vec3 color)
{
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma / (1. + luma);
    color *= toneMappedLuma / luma;
    color = pow(color, vec3(1. / GAMMA));
    return color;
}

void main() {
    vec3 HDR = texture(texSampler, texCoords).rgb;
    vec3 LDR = lumaBasedReinhardToneMapping(HDR);

    outColor = vec4(LDR, 1.0);
}