//
// Created by otrush on 3/3/2024.
//
#include "RenderStage.hpp"
#include "Graphics/Abstraction/Geometry/VertexStructures.hpp"

namespace shift::gfx {
    bool CreateRenderStageFromInfo(const Device& device, const ShiftBackBuffer& backBuff, DescriptorManager& descManager, RenderTargetManager& rtSystem, RenderStage &outStage, RenderStageCreateInfo info) {
        /// Filling base data
        outStage.name = info.name;
        outStage.viewSetLayoutType = info.viewSetLayoutType;
        outStage.matSetLayoutType = info.matSetLayoutType;

        switch (info.renderTargetType) {
            case RenderStageCreateInfo::RT_Type::Forward:
                // TODO: FUCKING FIX
                outStage.renderTargetFormats.push_back(VK_FORMAT_R16G16B16A16_SFLOAT);
                outStage.depthTargetFormat = rtSystem.GetDepthRT(RenderTargetManager::SWAPCHAIN_DEPTH).GetFormat();
                break;
            case RenderStageCreateInfo::RT_Type::Swapchain:
                // TODO: FUCKING FIX
                outStage.renderTargetFormats.push_back(backBuff.swapchain->GetFormat());
//                outStage.depthTargetFormat = rtSystem.GetDepthRT("Swaphain:Depth", 0).GetFormat();
                break;
        }

        outStage.pipeline = std::make_unique<shift::gfx::Pipeline>(device);

        std::string buildDir = util::GetShiftShaderBuildDir();
        std::array<std::unique_ptr<Shader>, 5> shaders;
        if (!info.shaderData[0].empty()) {
            shaders[0] = std::make_unique<Shader>(device, buildDir + info.shaderData[0], shift::gfx::Shader::Type::Vertex);
            if (!shaders[0]->CreateStage()) {}
            outStage.pipeline->AddShaderStage(*shaders[0]);
        }

        if (!info.shaderData[1].empty()) {
            shaders[1] = std::make_unique<Shader>(device, buildDir + info.shaderData[1], shift::gfx::Shader::Type::Fragment);
            if (!shaders[1]->CreateStage()) {}
            outStage.pipeline->AddShaderStage(*shaders[1]);
        }

        auto bindingDescription = gfx::Vertex::getBindingDescription();
        auto attributeDescriptions = gfx::Vertex::getAttributeDescriptions();
        if (info.renderTargetType != RenderStageCreateInfo::RT_Type::Swapchain) {
            outStage.pipeline->SetInputStateInfo(shift::info::CreateInputStateInfo(attributeDescriptions, {&bindingDescription, 1}));
        } else {
            outStage.pipeline->SetInputStateInfo(shift::info::CreateInputStateInfo({}, {}));
        }
        outStage.pipeline->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        outStage.pipeline->SetViewPortState();
        outStage.pipeline->SetDynamicState(info.dynamicStates);
        outStage.pipeline->SetRasterizerInfo(*info.rasterizerInfo);
        outStage.pipeline->SetDepthStencilInfo(*info.depthStencilStateInfo);
        outStage.pipeline->SetMultisampleInfo(*info.multisampleInfo);

        outStage.pipeline->SetBlendAttachment(*info.coloBlendAttInfo);
        outStage.pipeline->SetBlendState(info::CreateBlendStateInfo(*info.coloBlendAttInfo));

        switch (info.renderTargetType) {
            case RenderStageCreateInfo::RT_Type::Swapchain:
                outStage.pipeline->SetDynamicRenderingInfo(info::CreatePipelineRenderingInfo(outStage.renderTargetFormats,
                                                                                             {}));
                break;
            default:
                outStage.pipeline->SetDynamicRenderingInfo(info::CreatePipelineRenderingInfo(outStage.renderTargetFormats, outStage.depthTargetFormat));
        }

        std::array<VkDescriptorSetLayout, 3> sets{
                descManager.GetPerFrameLayout().Get(),
                descManager.GetPerViewLayout(outStage.viewSetLayoutType).Get(),
                descManager.GetPerMaterialLayout(outStage.matSetLayoutType).Get()
        };

        std::array<VkDescriptorSetLayout, 1> setsPP{
                descManager.GetPerMaterialLayout(outStage.matSetLayoutType).Get()
        };
        switch (info.renderTargetType) {
            case RenderStageCreateInfo::RT_Type::Swapchain:
                if (!outStage.pipeline->BuildLayout(setsPP)) {}
                break;
            default:
                if (!outStage.pipeline->BuildLayout(sets)) {}
        }

        return outStage.pipeline->Build();
    }
} // shift::gfx