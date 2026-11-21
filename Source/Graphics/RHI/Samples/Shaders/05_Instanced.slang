struct VSInput
{
    float2 position : TEXCOORD0;
    float2 instanceOffset : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

VSOutput mainVS(VSInput input, uint instanceId : SV_InstanceID)
{
    VSOutput output;
    output.position = float4(input.position + input.instanceOffset, 0.0, 1.0);
    output.color = float3((instanceId % 4) / 3.0, (instanceId / 4) / 3.0, 1.0 - instanceId / 15.0);
    return output;
}

float4 mainPS(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
