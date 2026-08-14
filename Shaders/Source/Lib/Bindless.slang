//! Bindless resource access
[[vk::binding(0, 0)]] SamplerState g_Samplers[];
[[vk::binding(1, 0)]] Texture2D g_Textures2D[];

//! Note: An index that VARIES per pixel (a material's texture slot) must be wrapped in
//! NonUniformResourceIndex at the call site. A compile-time constant like the placeholder's slot 0
//! is uniform by construction and needs nothing.
float4 SampleTexture2D(uint textureIdx, uint samplerIdx, float2 uv)
{
    return g_Textures2D[textureIdx].Sample(g_Samplers[samplerIdx], uv);
}
