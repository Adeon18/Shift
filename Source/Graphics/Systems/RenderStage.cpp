//
// Created by otrush on 3/3/2024.
//
#include "RenderStage.hpp"
#include "Graphics/Objects/VertexStructures.hpp"

namespace shift::gfx {
    bool CreateRenderStageFromInfo(const Device& device, const ShiftBackBuffer& backBuff, DescriptorManager& descManager , RenderStage &outStage, RenderStageCreateInfo info) {
        /// Filling base data
        outStage.viewSetLayoutType = info.viewSetLayoutType;
        outStage.matSetLayoutType = info.matSetLayoutType;

        switch (info.renderTargetType) {
            case RenderStageCreateInfo::RT_Type::Forward:
                outStage.renderTargetFormats.push_back(backBuff.swapchain->GetFormat());
                outStage.depthTargetFormat = backBuff.swapchain->GetDepthBufferFormat();
                break;
        }

        outStage.pipeline = std::make_unique<shift::gfx::Pipeline>(device);

        std::array<std::unique_ptr<Shader>, 5> shaders;
        if (!info.shaderData[0].empty()) {
            shaders[0] = std::make_unique<Shader>(device, info.shaderData[0], shift::gfx::Shader::Type::Vertex);
            if (!shaders[0]->CreateStage()) {}
            outStage.pipeline->AddShaderStage(*shaders[0]);
        }

        if (!info.shaderData[1].empty()) {
            shaders[1] = std::make_unique<Shader>(device, info.shaderData[1], shift::gfx::Shader::Type::Fragment);
            if (!shaders[1]->CreateStage()) {}
            outStage.pipeline->AddShaderStage(*shaders[1]);
        }

        auto bindingDescription = gfx::Vertex::getBindingDescription();
        auto attributeDescriptions = gfx::Vertex::getAttributeDescriptions();
        outStage.pipeline->SetInputStateInfo(shift::info::CreateInputStateInfo(attributeDescriptions, {&bindingDescription, 1}), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        outStage.pipeline->SetViewPortState();
        outStage.pipeline->SetDynamicState(info.dynamicStates);
        outStage.pipeline->SetRasterizerInfo(*info.rasterizerInfo);
        outStage.pipeline->SetDepthStencilInfo(*info.depthStencilStateInfo);
        outStage.pipeline->SetMultisampleInfo(*info.multisampleInfo);

        outStage.pipeline->SetBlendAttachment(*info.coloBlendAttInfo);
        outStage.pipeline->SetBlendState(info::CreateBlendStateInfo(*info.coloBlendAttInfo));

        outStage.pipeline->SetDynamicRenderingInfo(info::CreatePipelineRenderingInfo(outStage.renderTargetFormats, outStage.depthTargetFormat));

        std::array<VkDescriptorSetLayout, 3> sets{
                descManager.GetPerFrameLayout().Get(),
                descManager.GetPerViewLayout(outStage.viewSetLayoutType).Get(),
                descManager.GetPerMaterialLayout(outStage.matSetLayoutType).Get()
        };
        if (!outStage.pipeline->BuildLayout(sets)) {}

        return outStage.pipeline->Build();
    }
} // shift::gfx