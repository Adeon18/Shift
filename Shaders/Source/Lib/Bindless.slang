//! Bindless resource access
[[vk::binding(0, 0)]] SamplerState g_Samplers[];
[[vk::binding(1, 0)]] Texture2D g_Textures2D[];

//! textureIdx is uniform across a draw so nonuniform idx is not needed for now
float4 SampleTexture2D(uint textureIdx, uint samplerIdx, float2 uv)
{
    return g_Textures2D[textureIdx].Sample(g_Samplers[samplerIdx], uv);
}
