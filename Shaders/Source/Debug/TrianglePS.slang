import Lib.FrameData;
import Lib.Bindless;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("fragment")]
float4 mainPS(VertexOutput input) : SV_Target
{
    float4 texel = SampleTexture2D(0, SAMPLER_NEAREST_REPEAT, input.uv);
    return float4(texel.rgb, 1.0);
}
