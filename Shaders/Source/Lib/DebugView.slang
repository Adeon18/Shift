import Lib.FrameData;
import Lib.SurfaceInput;

static const uint MIP_RAMP_COUNT = 8;
static const float3 MIP_RAMP[MIP_RAMP_COUNT] = {
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.5f, 0.0f),
    float3(1.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 1.0f),
    float3(0.2f, 0.4f, 1.0f),
    float3(0.8f, 0.0f, 1.0f),
    float3(0.4f, 0.4f, 0.4f)
};

float3 DebugShade(EDebugViewMode mode, SurfaceInput surface)
{
    switch (mode)
    {
    case EDebugViewMode::Albedo:
        return surface.baseColor;
    case EDebugViewMode::Normal:
        return surface.normal * 0.5f + 0.5f;
    case EDebugViewMode::Roughness:
        return float3(surface.perceptualRoughness);
    case EDebugViewMode::Metallic:
        return float3(surface.metallic);
    case EDebugViewMode::Occlusion:
        return float3(surface.occlusion);
    case EDebugViewMode::UV:
        return float3(frac(surface.uv), 0.0f);
    case EDebugViewMode::Mip:
        return lerp(MIP_RAMP[uint(surface.mipLevel) % MIP_RAMP_COUNT], MIP_RAMP[(uint(surface.mipLevel) + 1) % MIP_RAMP_COUNT], frac(surface.mipLevel));
    default:
        return float3(1.0f, 0.0f, 1.0f);
    }
}
