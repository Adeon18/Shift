//! Also drawn by 04_Indexed
struct VSInput
{
    float2 position : TEXCOORD0;
    float3 color : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

VSOutput mainVS(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    return output;
}

float4 mainPS(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
