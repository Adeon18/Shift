//
// Created by otrush on 10/17/2026.
//

#include "LightIconSystem.hpp"

#include <array>

#include "Utility/Assertions.hpp"
#include "Utility/UtilStandard.hpp"

namespace Shift::Graphics {

    void LightIconSystem::Init(PipelineManager& pipelineManager, ETextureFormat outputFormat) {
        m_pipelineManager = &pipelineManager;

        PipelineDescriptor descriptor;
        descriptor.name = "LightIcon";

        ShaderDescriptor vsDescriptor;
        vsDescriptor.type = EShaderType::Vertex;
        vsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Editor/LightIcon.slang";
        vsDescriptor.entry = "mainVS";
        ShaderDescriptor fsDescriptor;
        fsDescriptor.type = EShaderType::Fragment;
        fsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Editor/LightIcon.slang";
        fsDescriptor.entry = "mainPS";

        descriptor.colorBlendConfig.attachments.push_back({.format = outputFormat});
        descriptor.rasterizerStateDesc.cullMode = ECullMode::None;
        descriptor.pushConstants = PushConstantRange{
            .offset = 0,
            .size = static_cast<uint32_t>(sizeof(GPU::LightIconPushConstants)),
            .stageFlags = EBindingVisibility::Vertex | EBindingVisibility::Fragment
        };
        std::array<ShaderDescriptor, 2> shaderSources{vsDescriptor, fsDescriptor};
        m_pipeline = m_pipelineManager->CreatePipeline(descriptor, shaderSources);
    }

    bool LightIconSystem::Record(RenderContextEncoder& encoder, const LightIconInputs& inputs) const {
        if (inputs.lightCount == 0) { return true; }

        Pipeline* pipeline = m_pipelineManager->Get(m_pipeline);
        CheckCritical(pipeline != nullptr, "The light icon pipeline does not resolve!");

        encoder.PushDebugGroup("LightIconPass", {0.95f, 0.85f, 0.35f, 1.0f}, true);
        encoder.TransitionTexture(inputs.output, EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

        const uint32_t width = inputs.output.GetWidth();
        const uint32_t height = inputs.output.GetHeight();

        RenderPassDescriptor pass;
        pass.colorAttachments.push_back({.loadOperation = EAttachmentLoadOperation::Load});
        pass.extent = {width, height};
        std::array targets{&inputs.output};
        encoder.BeginRenderPass(pass, targets, std::nullopt);

        encoder.SetScissor({{0, 0}, {width, height}});
        encoder.SetViewport({0.0f, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height), 0.0f, 1.0f});

        encoder.BindGraphicsPipeline(*pipeline);

        const GPU::LightIconPushConstants push{
            .frameConstantsRef = inputs.frameConstantsRef,
            .iconHalfSizeNDC = glm::vec2(inputs.iconPixelSize / static_cast<float>(width),
                                         inputs.iconPixelSize / static_cast<float>(height))
        };
        encoder.SetPushConstants(*pipeline, &push, static_cast<uint32_t>(sizeof(push)));
        encoder.Draw({.vertexCount = 6u * inputs.lightCount});

        encoder.EndRenderPass();

        encoder.TransitionTexture(inputs.output, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
        encoder.PopDebugGroup();

        return true;
    }
}
