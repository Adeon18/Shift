#ifndef TONE_MAPPING_OPERATORS_GLSL
#define TONE_MAPPING_OPERATORS_GLSL

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;
layout (set = 0, binding = 1) uniform PPUBO {
    vec4 data;
} UBO;

#define GAMMA 2.2

vec3 GammaCorrect(vec3 color, float gamma, float enable) {
    return mix(color, pow(color, vec3(1. / gamma)), enable);
}

vec3 AdjustExposure(vec3 color, float EV100, float enable)
{
    float LMax = (78.0f / (0.65f * 100.0f)) * pow(2.0f, EV100);
    return color * mix(1.0f, 1.0f / LMax, enable);
}

//! Base implementations taken from: https://www.shadertoy.com/view/sllXWr

vec3 ReinhardToneMapping(vec3 color)
{
    float exposure = 1.0;
    color *= exposure/(1. + color / exposure);

    return color;
}

vec3 LumaBasedReinhardToneMapping(vec3 color)
{
    float white = 2.;
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
    color *= toneMappedLuma / luma;

    return color;
}

vec3 RomBinDaHouseToneMapping(vec3 color)
{
    color = exp( -1.0 / ( 2.72*color + 0.15 ) );

    return color;
}

vec3 FilmicToneMapping(vec3 color)
{
    color = max(vec3(0.), color - vec3(0.004));
    color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
    color = pow(color, vec3(GAMMA));
    return color;
}

vec3 Uncharted2ToneMapping(vec3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    float exposure = 2.;
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;

    return color;
}

// ACES fitted
// from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

vec3 ACESFitted(vec3 color)
{
    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    const mat3 ACESOutputMat = mat3(
        1.60475, -0.53108, -0.07367,
        -0.10208,  1.10813, -0.00605,
        -0.00327, -0.07276,  1.07602
    );

    const mat3 ACESInputMat = mat3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );

    color = color * ACESInputMat;

    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    color = a / b;

    color = color * ACESOutputMat;
    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

// https://www.shadertoy.com/view/llXyWr
vec3 Tonemap_Lottes(vec3 color) {
    // Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
    const float a = 1.6;
    const float d = 0.977;
    const float hdrMax = 8.0;
    const float midIn = 0.18;
    const float midOut = 0.267;

    // Can be precomputed
    const float b =
    (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const float c =
    (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

    return pow(color, vec3(a)) / (pow(color, vec3(a * d)) * b + c);
}



#endif // TONE_MAPPING_OPERATORS_GLSL
