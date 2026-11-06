//! Bindless resource access
[[vk::binding(0, 0)]] SamplerState g_Samplers[];
[[vk::binding(1, 0)]] Texture2D g_Textures2D[];

//! textureIdx is uniform across a draw so nonuniform idx is not needed for now
float4 SampleTexture2D(uint textureIdx, uint samplerIdx, float2 uv)
{
    return g_Textures2D[textureIdx].Sample(g_Samplers[samplerIdx], uv);
}

float ComputeMipLevel(uint textureIdx, uint samplerIdx, float2 uv)
{
    return g_Textures2D[textureIdx].CalculateLevelOfDetail(g_Samplers[samplerIdx], uv);
}

float4 LoadTexture2D(uint textureIdx, int2 pixel)
{
    return g_Textures2D[textureIdx].Load(int3(pixel, 0));
}

//! Mip 0's size in texels
uint2 GetTexture2DSize(uint textureIdx)
{
    uint width;
    uint height;
    g_Textures2D[textureIdx].GetDimensions(width, height);
    return uint2(width, height);
}
