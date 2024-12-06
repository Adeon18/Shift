//
// Created by otrush on 3/3/2024.
//

#ifndef SHIFT_RENDERSTAGE_HPP
#define SHIFT_RENDERSTAGE_HPP

#include <memory>

#include "Graphics/Abstraction/Pipeline/Pipeline.hpp"
#include "Graphics/Abstraction/Images/Images.hpp"
#include "Graphics/Abstraction/DescriptorManager.hpp"
#include "Graphics/ShiftContextData.hpp"
#include "Graphics/Abstraction/Images/RenderTargetManager.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace Shift::gfx {
    struct RenderStage {
        std::string name;
        std::unique_ptr<Pipeline> pipeline;
        ViewSetLayoutType viewSetLayoutType;
        MaterialSetLayoutType matSetLayoutType;

        std::vector<VkFormat> renderTargetFormats;
        VkFormat depthTargetFormat;
    };

    //! A create struct for a render stage, is supposed to be stored before runtime, maybe serialized in future
    struct RenderStageCreateInfo {
        enum RT_Type {
            Forward,
            Gbuffer,
            Swapchain
        };
        std::string name;
        std::array<std::string, 5> shaderData;
        ViewSetLayoutType viewSetLayoutType;
        MaterialSetLayoutType matSetLayoutType;
        RT_Type renderTargetType;

        //! These structs below all have default values to abstact pipeline creation
        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        std::optional<VkPipelineRasterizationStateCreateInfo> rasterizerInfo = info::CreateRasterStateInfo();
        std::optional<VkPipelineMultisampleStateCreateInfo> multisampleInfo = info::CreateMultisampleStateInfo();
        std::optional<VkPipelineColorBlendAttachmentState> coloBlendAttInfo = Shift::info::CreateBlendAttachmentState();
        std::optional<VkPipelineDepthStencilStateCreateInfo> depthStencilStateInfo = info::CreateDepthStencilStateInfo();
    };

    bool CreateRenderStageFromInfo(const Device& device, const ShiftBackBuffer& backBuff, DescriptorManager& descManager, RenderTargetManager& rtManager, RenderStage &outStage, RenderStageCreateInfo info);
} // shift::gfx

#endif //SHIFT_RENDERSTAGE_HPP
