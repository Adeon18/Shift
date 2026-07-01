import TriangleInclude;

struct VertexOutput
{
    float4 position : SV_Position;
};

[shader("fragment")]
float4 mainPS(VertexOutput input) : SV_Target
{
    return float4(color.x, color.y, 1.0, 1.0);
}