//
// Created by otrush on 9/14/2026.
//

#include "ToneMapSystem.hpp"

#include <array>
#include <optional>

#include "Graphics/Managers/GlobalResourceSet.hpp"
#include "Graphics/Shared/GPUShared.h"
#include "Utility/Assertions.hpp"
#include "Utility/UtilStandard.hpp"

namespace Shift::Graphics {

    void ToneMapSystem::Init(PipelineManager& pipelineManager, ETextureFormat outputFormat) {
        m_pipelineManager = &pipelineManager;

        PipelineDescriptor descriptor;
        descriptor.name = "Tonemap";

        ShaderDescriptor vsDescriptor;
        vsDescriptor.type = EShaderType::Vertex;
        vsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "PostProcess/Tonemap.slang";
        vsDescriptor.entry = "mainVS";
        ShaderDescriptor fsDescriptor;
        fsDescriptor.type = EShaderType::Fragment;
        fsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "PostProcess/Tonemap.slang";
        fsDescriptor.entry = "mainPS";

        descriptor.colorBlendConfig.attachments.push_back({.format = outputFormat});
        descriptor.rasterizerStateDesc.cullMode = ECullMode::None;

        descriptor.pushConstants = PushConstantRange{
            .offset = 0,
            .size = static_cast<uint32_t>(sizeof(GPU::TonemapPushConstants)),
            .stageFlags = EBindingVisibility::Fragment
        };

        descriptor.descriptorLayouts.resize(GlobalResourceSet::SET_INDEX + 1);
        descriptor.descriptorLayouts[GlobalResourceSet::SET_INDEX] = GlobalResourceSet::Layout();

        std::array<ShaderDescriptor, 2> shaderSources{vsDescriptor, fsDescriptor};
        m_pipeline = m_pipelineManager->CreatePipeline(descriptor, shaderSources);
    }

    bool ToneMapSystem::Record(RenderContextEncoder& encoder, const ToneMapInputs& inputs) const {
        Pipeline* pipeline = m_pipelineManager->Get(m_pipeline);
        CheckCritical(pipeline != nullptr, "The tone map pipeline does not resolve!");

        encoder.PushDebugGroup("TonemapPass", {0.85f, 0.40f, 0.30f, 1.0f}, true);
        encoder.TransitionTexture(inputs.output, EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

        const uint32_t width = inputs.output.GetWidth();
        const uint32_t height = inputs.output.GetHeight();

        //! Nothing to clear
        RenderPassDescriptor pass;
        pass.colorAttachments.push_back({.loadOperation = EAttachmentLoadOperation::DontCare});
        pass.extent = {width, height};
        std::array targets{&inputs.output};
        encoder.BeginRenderPass(pass, targets, std::nullopt);

        encoder.SetScissor({{0, 0}, {width, height}});
        encoder.SetViewport({0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f});

        encoder.BindGraphicsPipeline(*pipeline);
        //! Slightly different global set 1
        encoder.BindResourceSet(*pipeline, GlobalResourceSet::SET_INDEX, inputs.globalSet);

        const GPU::TonemapPushConstants push{
            .frameConstantsRef = inputs.frameConstantsRef,
            .hdrColorTex = inputs.hdrColorSlot
        };
        encoder.SetPushConstants(*pipeline, &push, static_cast<uint32_t>(sizeof(push)));

        encoder.Draw({.vertexCount = 3});

        encoder.EndRenderPass();

        encoder.TransitionTexture(inputs.output, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
        encoder.PopDebugGroup();

        return true;
    }
}
