//! Tonemap pass
import Lib.FrameData;
import Lib.Bindless;

[[vk::push_constant]] TonemapPushConstants pc;

struct TonemapData
{
    float4 position : SV_Position;
};

// https://www.shadertoy.com/view/lcdXz4
float3 OpReinhard(float3 color)
{
    float white = 0.8;
    float3 a = color * (1.0 + (color / (white * white)));
    float3 b = 1.0 + color;
    return a / b;
}

// https://www.shadertoy.com/view/lcdXz4
float3 OpUncharted(float3 color)
{
     float A = 0.15;
     float B = 0.50;
     float C = 0.10;
     float D = 0.20;
     float E = 0.02;
     float F = 0.30;
     float W = 11.2;

     float3 R = color * 2.0;
     float3 cA =  ((R*(A*R+C*B)+D*E)/(R*(A*R+B)+D*F))-E/F;
     float cB = ((W*(A*W+C*B)+D*E)/(W*(A*W+B)+D*F))-E/F;
     return cA / cB;
}

// https://www.shadertoy.com/view/lcdXz4
float3 OpACES(float3 color)
{
    float3x3 ACESInputMat = float3x3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );

    float3x3 ACESOutputMat = float3x3(
         1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
    );

    float3x3 ACESFixMat = float3x3(
         0.9404372683, 0.0083786969,  0.0005471261,
        -0.0183068787, 0.8286599939, -0.0008833746,
         0.0778696104, 0.1629613092,  1.0003362486
    );

    float3 x = mul(color, ACESFixMat);
    float3 v = mul(x, ACESInputMat);
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    float3 c = mul((a / b), ACESOutputMat);
    return c;
}

float3 ApplyTonemap(float3 color, ETonemapOperator tonemapOperator)
{
    switch (tonemapOperator)
    {
    case ETonemapOperator::Reinhard:  return OpReinhard(color);
    case ETonemapOperator::Uncharted: return OpUncharted(color);
    case ETonemapOperator::ACES:      return OpACES(color);
    default:                          return saturate(color);
    }
}

[shader("vertex")]
TonemapData mainVS(uint vertexID : SV_VertexID)
{
    float2 corner = float2(float((vertexID << 1) & 2), float(vertexID & 2));

    TonemapData output;
    output.position = float4(corner * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 mainPS(TonemapData input) : SV_Target
{
    FrameConstants frame = LoadFrameConstants(pc.frameConstantsRef);

    float3 hdr = LoadTexture2D(pc.hdrColorTex, int2(input.position.xy)).rgb;
    float3 exposed = hdr * frame.cameraPosExposure.w;

    return float4(ApplyTonemap(exposed, pc.tonemapOperator), 1.0f);
}
