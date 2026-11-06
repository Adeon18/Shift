//
// Created by otrush on 11/4/2026.
//

#include "AutoExposureSystem.hpp"

#include <array>
#include <bit>

#include "Graphics/Managers/GlobalResourceSet.hpp"
#include "Graphics/Shared/GPUShared.h"
#include "Utility/Assertions.hpp"
#include "Utility/UtilStandard.hpp"

namespace Shift::Graphics {
    namespace {
        constexpr uint64_t HISTOGRAM_BYTES = GPU::AUTO_EXPOSURE_HISTOGRAM_BINS * sizeof(uint32_t);
        constexpr uint64_t EXPOSURE_BYTES = sizeof(float);

        ShaderDescriptor ComputeStage(const char* entry) {
            ShaderDescriptor stage;
            stage.type = EShaderType::Compute;
            stage.path = Shift::Util::GetShiftShaderSrcDir() + "PostProcess/AutoExposure.slang";
            stage.entry = entry;
            return stage;
        }
    }

    bool AutoExposureSystem::Init(PipelineManager& pipelineManager, BufferManager& bufferManager, RenderContextEncoder& initEncoder, float seedExposure) {
        m_pipelineManager = &pipelineManager;
        m_bufferManager = &bufferManager;

        //! Pass 1: Build histogram
        {
            PipelineDescriptor descriptor;
            descriptor.name = "AutoExposureBuildHistogram";
            descriptor.pushConstants = PushConstantRange{
                .offset = 0,
                .size = static_cast<uint32_t>(sizeof(GPU::HistogramBuildPushConstants)),
                .stageFlags = EBindingVisibility::Compute
            };
            descriptor.descriptorLayouts.resize(GlobalResourceSet::SET_INDEX + 1);
            descriptor.descriptorLayouts[GlobalResourceSet::SET_INDEX] = GlobalResourceSet::Layout();

            const std::array stages{ComputeStage("BuildHistogramCS")};
            m_buildPipeline = m_pipelineManager->CreatePipeline(descriptor, stages);
        }
        //! Pass 2: Reduce
        {
            PipelineDescriptor descriptor;
            descriptor.name = "AutoExposureReduce";
            descriptor.pushConstants = PushConstantRange{
                .offset = 0,
                .size = static_cast<uint32_t>(sizeof(GPU::HistogramReducePushConstants)),
                .stageFlags = EBindingVisibility::Compute
            };

            const std::array stages{ComputeStage("HistogramReduceCS")};
            m_reducePipeline = m_pipelineManager->CreatePipeline(descriptor, stages);
        }
        CheckCritical(m_pipelineManager->Get(m_buildPipeline) != nullptr, "The auto-exposure build pipeline was not created!");
        CheckCritical(m_pipelineManager->Get(m_reducePipeline) != nullptr, "The auto-exposure reduce pipeline was not created!");

        m_histogram = m_bufferManager->CreateBuffer({
            .size = HISTOGRAM_BYTES,
            .name = "AutoExposureHistogram",
            .type = EBufferType::Storage,
            .isDeviceAddressable = true
        });
        m_exposure = m_bufferManager->CreateBuffer({
            .size = EXPOSURE_BYTES,
            .name = "AutoExposureValue",
            .type = EBufferType::Storage,
            .isDeviceAddressable = true
        });
        Buffer* histogram = m_bufferManager->Get(m_histogram);
        Buffer* exposure = m_bufferManager->Get(m_exposure);
        CheckCritical(histogram != nullptr && exposure != nullptr, "Failed to create the auto-exposure buffers!");

        //! Zero out histogram and set the starting exposure value
        initEncoder.FillBuffer({histogram, 0}, HISTOGRAM_BYTES, 0u);
        initEncoder.FillBuffer({exposure, 0}, EXPOSURE_BYTES, std::bit_cast<uint32_t>(seedExposure));

        return true;
    }

    bool AutoExposureSystem::Record(RenderContextEncoder& encoder, const AutoExposureInputs& inputs) const {
        const Pipeline* build = m_pipelineManager->Get(m_buildPipeline);
        const Pipeline* reduce = m_pipelineManager->Get(m_reducePipeline);
        CheckCritical(build != nullptr && reduce != nullptr, "The auto-exposure pipelines do not resolve!");
        Buffer* histogram = m_bufferManager->Get(m_histogram);
        Buffer* exposure = m_bufferManager->Get(m_exposure);
        CheckCritical(histogram != nullptr && exposure != nullptr, "The auto-exposure buffers do not resolve!");

        encoder.PushDebugGroup("AutoExposure", {0.95f, 0.75f, 0.20f, 1.0f}, true);
        RecordBuild(encoder, *build, *histogram, *exposure, inputs);
        RecordReduce(encoder, *reduce, *histogram, *exposure, inputs);
        encoder.PopDebugGroup();

        return true;
    }

    void AutoExposureSystem::RecordBuild(RenderContextEncoder& encoder, const Pipeline& pipeline, Buffer& histogram, Buffer& exposure, const AutoExposureInputs& inputs) const {
        encoder.PushDebugGroup("AEBuild", {0.1f, 0.9f, 0.8f, 1.0f}, true);
        encoder.BufferBarrier({&histogram, 0}, histogram.GetSize(), {
            .srcStage = EPipelineStageFlags::ComputeShaderBit,
            .srcAccess = EAccessFlags::ShaderStorageWriteBit,
            .dstStage = EPipelineStageFlags::ComputeShaderBit,
            .dstAccess = EAccessFlags::ShaderStorageReadBit | EAccessFlags::ShaderStorageWriteBit
        });

        encoder.BindComputePipeline(pipeline);
        encoder.BindResourceSet(pipeline, GlobalResourceSet::SET_INDEX, inputs.globalSet);
        const GPU::HistogramBuildPushConstants ps{
            .histogramRef = histogram.GetDeviceAddress(),
            .exposureRef = exposure.GetDeviceAddress(),
            .hdrColorTex = inputs.hdrColorSlot,
        };
        encoder.SetPushConstants(pipeline, &ps, sizeof(GPU::HistogramBuildPushConstants), 0);
        encoder.Dispatch(DispatchConfig::CoveringThreads(inputs.width, inputs.height, 1, GPU::AUTO_EXPOSURE_GROUP_DIM, GPU::AUTO_EXPOSURE_GROUP_DIM, 1));
        encoder.PopDebugGroup();
    }

    void AutoExposureSystem::RecordReduce(RenderContextEncoder& encoder, const Pipeline& pipeline, Buffer& histogram, Buffer& exposure, const AutoExposureInputs& inputs) const {
        encoder.PushDebugGroup("AEReduce", {0.44f, 0.9f, 0.20f, 1.0f}, true);

        encoder.BufferBarrier({&histogram, 0}, histogram.GetSize(), {
            .srcStage = EPipelineStageFlags::ComputeShaderBit,
            .srcAccess = EAccessFlags::ShaderStorageWriteBit,
            .dstStage = EPipelineStageFlags::ComputeShaderBit,
            .dstAccess = EAccessFlags::ShaderStorageReadBit | EAccessFlags::ShaderStorageWriteBit
        });
        encoder.BufferBarrier({&exposure, 0}, exposure.GetSize(), {
            .srcStage = EPipelineStageFlags::ComputeShaderBit | EPipelineStageFlags::FragmentShaderBit,
            .srcAccess = EAccessFlags::NoneBit,
            .dstStage = EPipelineStageFlags::ComputeShaderBit,
            .dstAccess = EAccessFlags::NoneBit
        });

        encoder.BindComputePipeline(pipeline);
        const GPU::HistogramReducePushConstants ps{
            .histogramRef = histogram.GetDeviceAddress(),
            .exposureRef = exposure.GetDeviceAddress(),
            .pixelCount = inputs.height * inputs.width,
            .deltaTime = inputs.deltaTime,
            .adaptationSpeed = inputs.adaptationSpeed,
            .minExposure = inputs.minExposure,
            .maxExposure = inputs.maxExposure
        };
        encoder.SetPushConstants(pipeline, &ps, sizeof(GPU::HistogramReducePushConstants), 0);
        encoder.Dispatch(DispatchConfig{1, 1, 1});

        encoder.BufferBarrier({&exposure, 0}, exposure.GetSize(), {
            .srcStage = EPipelineStageFlags::ComputeShaderBit,
            .srcAccess = EAccessFlags::ShaderStorageWriteBit,
            .dstStage = EPipelineStageFlags::ComputeShaderBit | EPipelineStageFlags::FragmentShaderBit,
            .dstAccess = EAccessFlags::ShaderStorageReadBit
        });

        encoder.PopDebugGroup();
    }

    uint64_t AutoExposureSystem::GetExposureRef() const {
        const Buffer* exposure = m_bufferManager->Get(m_exposure);
        return exposure != nullptr ? exposure->GetDeviceAddress() : 0;
    }

    BufferCaptureTarget AutoExposureSystem::GetHistogramCaptureTarget() const {
        return {m_bufferManager->Get(m_histogram), HISTOGRAM_BYTES, EPipelineStageFlags::ComputeShaderBit, EAccessFlags::ShaderStorageWriteBit};
    }

    BufferCaptureTarget AutoExposureSystem::GetExposureCaptureTarget() const {
        return {m_bufferManager->Get(m_exposure), EXPOSURE_BYTES, EPipelineStageFlags::ComputeShaderBit, EAccessFlags::ShaderStorageWriteBit};
    }
}
