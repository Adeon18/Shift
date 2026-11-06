//! Auto-exposure via histogram, Tardif
import Lib.FrameData;
import Lib.Bindless;
import Lib.BufferAccess;

static const float EPSILON = 1e-8;
//! They got it from CIE
static const float MIDDLE_GREY = 0.18f;

[[vk::push_constant]] HistogramBuildPushConstants buildPc;
[[vk::push_constant]] HistogramReducePushConstants reducePc;

//! Pass 1
groupshared uint histogramShared[AUTO_EXPOSURE_HISTOGRAM_BINS];

//! Pass 2
groupshared float histogramWeighedShared[AUTO_EXPOSURE_HISTOGRAM_BINS];

uint HDRToHistogramBin(float3 hdrColor, float appliedLog2Exposure)
{
    float lum = dot(hdrColor, float3(0.2125f, 0.7154f, 0.0721f));
    if (lum < EPSILON) {
        return 0;
    }

    //! Since the image is preexposed, unexpose it via applied log 2 exposure
    float logLum = clamp((log2(lum) - appliedLog2Exposure - AUTO_EXPOSURE_MIN_LOG2_LUM) / AUTO_EXPOSURE_LOG2_LUM_RANGE, 0.0, 1.0);

    //! [0, 1] -> [1, 255]
    return uint(logLum * 254.0 + 1.0);
}

[shader("compute")]
[numthreads(AUTO_EXPOSURE_GROUP_DIM, AUTO_EXPOSURE_GROUP_DIM, 1)]
void BuildHistogramCS(uint groupIndex : SV_GroupIndex, uint3 threadID : SV_DispatchThreadID)
{
    histogramShared[groupIndex] = 0;
    GroupMemoryBarrierWithGroupSync();

    uint2 textureDims = GetTexture2DSize(buildPc.hdrColorTex);
    if (threadID.x < textureDims.x && threadID.y < textureDims.y) {
        float3 HDRColor = LoadTexture2D(buildPc.hdrColorTex, threadID.xy).xyz;
        InterlockedAdd(histogramShared[HDRToHistogramBin(HDRColor, log2(Load<float>(buildPc.exposureRef)))], 1);
    }

    GroupMemoryBarrierWithGroupSync();
    AtomicAddAt(buildPc.histogramRef, groupIndex, histogramShared[groupIndex]);
}

[shader("compute")]
[numthreads(AUTO_EXPOSURE_GROUP_DIM, AUTO_EXPOSURE_GROUP_DIM, 1)]
void HistogramReduceCS(uint groupIndex : SV_GroupIndex)
{
    uint currentBinCount = LoadAt<uint>(reducePc.histogramRef, groupIndex);
    histogramWeighedShared[groupIndex] = currentBinCount * groupIndex;

    GroupMemoryBarrierWithGroupSync();

    StoreAt<uint>(reducePc.histogramRef, groupIndex, 0);

    for (uint indexCutoff = AUTO_EXPOSURE_HISTOGRAM_BINS >> 1; indexCutoff > 0; indexCutoff >>= 1) {
        if (groupIndex < indexCutoff) {
            histogramWeighedShared[groupIndex] += histogramWeighedShared[groupIndex + indexCutoff];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupIndex == 0) {
        float weightedLogAvg = (histogramWeighedShared[0] / max(reducePc.pixelCount - float(currentBinCount), 1.0)) - 1.0;
        float weightedAvgLum = exp2(((weightedLogAvg / 254.0f) * AUTO_EXPOSURE_LOG2_LUM_RANGE) + AUTO_EXPOSURE_MIN_LOG2_LUM);
        //! The goal is to make the average luminance land at middle grey which is 0.18 as some smart people figured
        float targetLog2Exposure = clamp(log2(MIDDLE_GREY) - log2(weightedAvgLum), reducePc.minExposure, reducePc.maxExposure);
        float lastLog2Exposure = log2(Load<float>(reducePc.exposureRef));
        float adaptedLog2Exposure = lastLog2Exposure + (targetLog2Exposure - lastLog2Exposure) * (1.0f - exp(-reducePc.adaptationSpeed * reducePc.deltaTime));
        Store<float>(reducePc.exposureRef, exp2(adaptedLog2Exposure));
    }
}
