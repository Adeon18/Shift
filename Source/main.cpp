// #include <vulkan/vulkan.h> This is fine for offscreen rendering but for window rendering we do as below

#include "VertexStructures.hpp"
#include "UniformBufferStructs.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Utility/UtilStandard.hpp"
#include "Utility/Vulkan/UtilVulkan.hpp"
#include "Utility/Vulkan/InfoUtil.hpp"

#include "Window/ShiftWindow.hpp"
#include "Graphics/Device/WindowSurface.hpp"
#include "Graphics/Device/Instance.hpp"
#include "Graphics/Device/Device.hpp"
#include "Graphics/Device/Swapchain.hpp"
#include "Graphics/Synchronization/Fence.hpp"
#include "Graphics/Synchronization/Semaphore.hpp"

#include <spdlog/spdlog.h>

//! TODO: Can be optimized!
static std::vector<char> readFile(const std::string& filename) {
    // We start reading at the end in order to get the file size
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

class HelloTriangleApplication {
    static constexpr uint32_t WINDOW_WIDTH = 800;
    static constexpr uint32_t WINDOW_HEIGHT = 600;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
    static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

public:
    void run() {
        initWindow();
        if (!initVulkan()) {
            cleanup();
            return;
        }
        mainLoop();
        cleanup();
    }

private:

private:
    void initWindow() {
        m_winPtr = std::make_unique<sft::ShiftWindow>(WINDOW_WIDTH, WINDOW_HEIGHT, "Shift");
        spdlog::set_level(spdlog::level::debug);
    }

    bool initVulkan() {
        createVkInstance();

        createSurface();

        m_device = std::make_unique<sft::gfx::Device>(*m_instance, m_surfacePtr->Get());

        if (!createSwapchain()) {return false;}

        // MUST BE CREATED BEFORE PIPELINE
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPools();

        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();

        createCommandBuffers();
        createSyncObjects();
        return true;
    }

    //! Basically wer make an app info struct
    void createVkInstance() {
        m_instance = std::make_unique<sft::gfx::Instance>("TestApp", VK_MAKE_VERSION(1, 0, 0), "ShiftEngine",
                                                          VK_MAKE_VERSION(1, 0, 0));
    }

    void createSurface() {
        m_surfacePtr = std::make_unique<sft::gfx::WindowSurface>(m_instance->Get(), m_winPtr->GetHandle());
    }

    bool createSwapchain() {
        m_swapchain = std::make_unique<sft::gfx::Swapchain>(*m_device, *m_surfacePtr, m_winPtr->GetWidth(), m_winPtr->GetHeight());
        return m_swapchain->IsValid();
    }

    //! INFO WARNING UTILITY TODO We don't recreate the renderpass here, even though we should because the windoe might be moving to another
    //! screen which might be HDR so the image format might change
    bool recreateSwapChain() {
        vkDeviceWaitIdle(m_device->Get());
        // Wait for device to stop using resources
        cleanupSwapChain();

        if (!m_swapchain->Recreate(m_winPtr->GetWidth(), m_winPtr->GetHeight())) return false;

        createFramebuffers();
        return true;
    }

    void cleanupSwapChain() {
        for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(m_device->Get(), m_swapChainFramebuffers[i], nullptr);
        }
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;   // It is possible for the shader to represent an array of UBOs
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   // Shader type, can specify all
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;   // It is possible for the shader to represent an array of UBOs
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;   // Shader type, can specify all
        samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_device->Get(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    //! TODO: Be aware that you might need to clear vectors after module creation to free up memory
    void createGraphicsPipeline() {
        auto vertShaderCode = readFile(sft::utl::GetShiftRoot() + "Shaders/shader.vert.spv");
        auto fragShaderCode = readFile(sft::utl::GetShiftRoot() + "Shaders/shader.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        // You can set constant explicitly whic alloes vulkan to optimize shader code based on the constants
        vertShaderStageInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        // No vertex data for now
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Same as in DX yooooooooo
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchain->GetExtent();

        //! This is basically most of the dunamic states that you can change at runtime
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;         // Clamps objects to min/max depth instead of discard
        rasterizer.rasterizerDiscardEnable = VK_FALSE;  // If enabled, the fragments will mever pass on to the framebuffer
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional


        // Disable multisampling for now
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // These are global constants
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        // These are local per framebuffer constants
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // You can specify mode than 1 layouts, why tho
        pipelineLayoutInfo.setLayoutCount = 1; // Optional
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(m_device->Get(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        // Fixed function stage
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = m_pipelineLayout; // Specify the layout of the pipeline
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;               // Which subpass is this pipeline suitable for

        // You can derive pipelines with the help of these parameters
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(m_device->Get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(m_device->Get(), fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device->Get(), vertShaderModule, nullptr);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        // The pointer to bytecode is a pointer to const uint32_t
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_device->Get(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swapchain->GetFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;    // No multisampling
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Clear before render
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store data after render

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // Defines what to do with stencil data
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // We will clear the image so initial layout does not matter
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // Images to be presented in the swap chain

        //! INFO: Basically here we create an attachment that we render out output into via layout command
        //! As for the subpasses, if we had a deferred rendered, we could make it consist of 2 subpasses
        //! but right now we only have a single subpass
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // NEED TODO: UNDERSTAND THIS
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device->Get(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createFramebuffers() {
        auto& swapImgViews = m_swapchain->GetImageViews();
        m_swapChainFramebuffers.resize(swapImgViews.size());

        for (size_t i = 0; i < swapImgViews.size(); i++) {
            VkImageView attachments[] = {
                    swapImgViews[i]
            };

            //! INFO: YOU CAN ONLY USE THE FRAMEBUFFER WITH THE COMPATIBLE RENDERPASS
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapchain->GetExtent().width;
            framebufferInfo.height = m_swapchain->GetExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device->Get(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPools() {
        sft::gutil::QueueFamilyIndices queueFamilyIndices = sft::gutil::FindQueueFamilies(m_device->GetPhysicalDevice(), m_surfacePtr->Get());

        VkCommandPoolCreateInfo poolInfoGraphics{};
        poolInfoGraphics.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfoGraphics.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfoGraphics.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        //! INFO: Command buffers are executed by submitting them on one of the device queues, like the graphics
        //! and presentation queues we retrieved.Each command pool can only allocate command buffers that are
        //! submitted on a single type of queue.We're going to record commands for drawing, which is why we've chosen the graphics queue family.
        if (vkCreateCommandPool(m_device->Get(), &poolInfoGraphics, nullptr, &m_commandPoolGraphics) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }

        VkCommandPoolCreateInfo poolInfoTransfer{};
        poolInfoTransfer.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfoTransfer.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfoTransfer.queueFamilyIndex = queueFamilyIndices.transferFamily.value();

        if (vkCreateCommandPool(m_device->Get(), &poolInfoTransfer, nullptr, &m_commandPoolTransfer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("../assets/skeleton.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 bytes per pixel

        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        // Copy data to staging buffer
        void* data;
        vkMapMemory(m_device->Get(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_device->Get(), stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(
            texWidth,
            texHeight,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_textureImage,
            m_textureImageMemory
        );

        transitionImageLayout(m_commandPoolTransfer, m_device->GetTransferQueue(), m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(m_commandPoolTransfer, m_device->GetTransferQueue(), stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        transitionImageLayout(m_commandPoolGraphics, m_device->GetGraphicsQueue(), m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_device->Get(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->Get(), stagingBufferMemory, nullptr);
    }

    void createTextureImageView() {
        m_textureImageView = m_device->CreateImageView(sft::info::CreateImageViewInfo(
                m_textureImage, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB
                ));
    }

    void createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_device->GetPhysicalDevice(), &properties);
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        // You can either use UVs or [0, width/height] coordinates
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        // This is for PCF
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        // This is mipmapping information
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(m_device->Get(), &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_device->Get(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device->Get(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device->Get(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(m_device->Get(), image, imageMemory, 0);
    }

    void createVertexBuffer() {
        
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // Use a staging buffer, the usage - can be used as source during memory transfer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        
        //! INFO: The driver may not immediately copy the data into the buffer memory, so VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        //! is needed to do that or some other fancy shit like flushing the memory with your hands
        void* data;
        vkMapMemory(m_device->Get(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(m_device->Get(), stagingBufferMemory);

        // The vertex buffer can also be used as dest in memory transfer
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

        // Destroy staging buffer
        vkDestroyBuffer(m_device->Get(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->Get(), stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        //! INFO: The driver may not immediately copy the data into the buffer memory, so VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        //! is needed to do that or some other fancy shit like flushing the memory with your hands
        void* data;
        vkMapMemory(m_device->Get(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(m_device->Get(), stagingBufferMemory);

        // The vertex buffer can also be used as dest in memory transfer
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

        // Destroy staging buffer
        vkDestroyBuffer(m_device->Get(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->Get(), stagingBufferMemory, nullptr);

    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(PerFrame);

        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // The technique is called PERSISTENT MAPPING, the buffer is mapped during the entire duration and we do not map
            // it every frame, it is faster this way
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);

            vkMapMemory(m_device->Get(), m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        // INFO: Btw the driver may not allocate the descrioptorCount amount of descriptors, but it is still the best practice to do so in infostruct
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(m_device->Get(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        // INFO: When allocating a descriptor set you neeed to: specify the pool to allocate from, the number of descriptor sets to allocate,
        // the descriptor layout to base them on
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_device->Get(), &allocInfo, m_descriptorSets.data())) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        // We now need to populate every descriptor set
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // This is the struct for the buffer information
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(PerFrame);    // INFO: If we are entirely rewriting the buffer, it is possible to use the VK_WHOLE_SIZE
            // This is the struct for the image information
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_textureImageView;
            imageInfo.sampler = m_textureSampler;

            // To update the sets we need this struct
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};;
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;

            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;    // How many(if usiing array) of array elements to update

            descriptorWrites[0].pBufferInfo = &bufferInfo;  // Buffer data descriptors
            descriptorWrites[0].pImageInfo = nullptr; // Optional: Image Data
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional: View Data

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_device->Get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;         // No need to share the buffer

        if (vkCreateBuffer(m_device->Get(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        // We need to allocate memory for the buffer
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device->Get(), buffer, &memRequirements);

        // Get the memorytypes size and reauirements and check if we can map it and write to it
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device->Get(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(m_device->Get(), buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        // Create a temporary command pool for such commands
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_commandPoolTransfer);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer, m_commandPoolTransfer, m_device->GetTransferQueue());
    }

    void transitionImageLayout(VkCommandPool commandPool, VkQueue queue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // This is filled only if we use it to do queue ffamily ownership transfer
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // Handle transitions UNDEFINED -> Dest
        // Dest -> Shader Reading
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer, commandPool, queue);
    }

    void copyBufferToImage(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        endSingleTimeCommands(commandBuffer, commandPool, queue);
    }

    //! INFO: GPUs can offer different memory types => we need to find the right one
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        // Constaine memory types and memory heaps, heap is VRAM, type is types of memory in that VRAM
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_device->GetPhysicalDevice(), &memProperties);

        // Out memory is shosen by index and we need for it to support specific properties
        // bitwise & because we need to have ALL desired properties
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers() {
        m_commandBuffersGraphics.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfoGraphics{};
        allocInfoGraphics.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfoGraphics.commandPool = m_commandPoolGraphics;
        allocInfoGraphics.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Primary can be submitted to the queue, secondary can be called from primary buffers and inversely
        allocInfoGraphics.commandBufferCount = static_cast<uint32_t>(m_commandBuffersGraphics.size());

        if (vkAllocateCommandBuffers(m_device->Get(), &allocInfoGraphics, m_commandBuffersGraphics.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_device->Get(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(m_device->Get(), commandPool, 1, &commandBuffer);
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        //! Begin a command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        //! Begin a render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchain->GetExtent();

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // Since the viewport and scissor are dynamic, we must set them here
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchain->GetExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind the descriptor sets
        // INFO: The sets are not unique to pipelines, so we need to specify whether to bind it to compute pipeline or the graphics one
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);

        // DRAW THE FUCKING TRIANGLE
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i] = std::make_unique<sft::gfx::Semaphore>(*m_device);
            m_renderFinishedSemaphores[i] = std::make_unique<sft::gfx::Semaphore>(*m_device);
            m_inFlightFences[i] = std::make_unique<sft::gfx::Fence>(*m_device, true);
        }
    }

    void mainLoop() {
        while (m_winPtr->IsActive()) {
            m_winPtr->Process();
            if (!drawFrame()) break;
        }

        //! Wait for device to finish operations so we can clean everything properly
        vkDeviceWaitIdle(m_device->Get());
    }

    bool drawFrame() {
        // Wait for end of previous frame and then reset it
        m_inFlightFences[m_currentFrame]->Wait();

        // Aquire availible swapchain image index
        bool changed = false;
        uint32_t imageIndex = m_swapchain->AquireNextImageIndex(*m_imageAvailableSemaphores[m_currentFrame], &changed);

        if (imageIndex == UINT32_MAX) return false;
        if (changed) {
            if (!recreateSwapChain()) return false;
            return true;
        }

        // Only reset fence if work is submitted
        m_inFlightFences[m_currentFrame]->Reset();

        // Reset the command buffer
        vkResetCommandBuffer(m_commandBuffersGraphics[m_currentFrame], 0);

        recordCommandBuffer(m_commandBuffersGraphics[m_currentFrame], imageIndex);

        updateUniformBuffer(m_currentFrame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame]->Get() };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        // Which command buffer to use
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffersGraphics[m_currentFrame];

        // Which semaphores to wait for after render is finished
        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame]->Get() };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]->Get()) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        bool isOld = false;
        bool success = m_swapchain->Present(*m_renderFinishedSemaphores[m_currentFrame], imageIndex, &isOld);
        if (!success) {return false;}

        if (isOld || m_winPtr->ShouldProcessResize()) {
            m_winPtr->ProcessResize();
            if (!recreateSwapChain()) return false;
        }

        // Update the current frame
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return true;
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        PerFrame pf{};
        pf.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //pf.model = glm::mat4(1.0f);

        pf.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //pf.view = glm::mat4(1.0f);

        pf.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_swapchain->GetExtent().width) / static_cast<float>(m_swapchain->GetExtent().height), 0.1f, 10.0f);
        //pf.proj = glm::mat4(1.0f);

        // Y coordinate is fliped in Vulkan??
        pf.proj[1][1] *= -1;

        memcpy(m_uniformBuffersMapped[currentImage], &pf, sizeof(pf));

        // INFO: Actually a more efficient way to use these buffers are not mapping them during the entire runtime, but rather using push constants
    }

    void cleanup() {

        cleanupSwapChain();
        m_swapchain.reset();

        vkDestroySampler(m_device->Get(), m_textureSampler, nullptr);

        vkDestroyImageView(m_device->Get(), m_textureImageView, nullptr);
        vkDestroyImage(m_device->Get(), m_textureImage, nullptr);
        vkFreeMemory(m_device->Get(), m_textureImageMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(m_device->Get(), m_uniformBuffers[i], nullptr);
            vkFreeMemory(m_device->Get(), m_uniformBuffersMemory[i], nullptr);
        }

        // INFO: The descriptor sets are destroyed with the descriptor pool
        vkDestroyDescriptorPool(m_device->Get(), m_descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(m_device->Get(), m_descriptorSetLayout, nullptr);

        vkDestroyBuffer(m_device->Get(), m_vertexBuffer, nullptr);
        vkFreeMemory(m_device->Get(), m_vertexBufferMemory, nullptr);

        vkDestroyBuffer(m_device->Get(), m_indexBuffer, nullptr);
        vkFreeMemory(m_device->Get(), m_indexBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i].reset();
            m_renderFinishedSemaphores[i].reset();
            m_inFlightFences[i].reset();
        }
        vkDestroyCommandPool(m_device->Get(), m_commandPoolGraphics, nullptr);
        vkDestroyCommandPool(m_device->Get(), m_commandPoolTransfer, nullptr);

        vkDestroyPipeline(m_device->Get(), m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device->Get(), m_pipelineLayout, nullptr);
        vkDestroyRenderPass(m_device->Get(), m_renderPass, nullptr);

        m_surfacePtr.reset();
        m_device.reset();
    }
private:

    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;
    // All describto binding are located here
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkPipelineLayout m_pipelineLayout;

    std::unique_ptr<sft::ShiftWindow> m_winPtr;
    std::unique_ptr<sft::gfx::Device> m_device;
    std::unique_ptr<sft::gfx::WindowSurface> m_surfacePtr;
    std::unique_ptr<sft::gfx::Instance> m_instance;
    std::unique_ptr<sft::gfx::Swapchain> m_swapchain;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    // INFO: You have to record all draw operations in command buffers, for that you need a command pool
    VkCommandPool m_commandPoolGraphics;
    VkCommandPool m_commandPoolTransfer;
    std::vector<VkCommandBuffer> m_commandBuffersGraphics;

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;

    // Texture data
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;

    // We need a couple of uniform buffers because we have multiple frames in fright so that we do not write to a frame that
    // the GPU is reading from currently
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    // Sync primitives to comtrol the rendering of a frame
    std::vector<std::unique_ptr<sft::gfx::Semaphore>> m_imageAvailableSemaphores;
    std::vector<std::unique_ptr<sft::gfx::Semaphore>> m_renderFinishedSemaphores;
    std::vector<std::unique_ptr<sft::gfx::Fence>> m_inFlightFences;

    uint32_t m_currentFrame = 0;
};

int main() {
    HelloTriangleApplication app;

    app.run();

    return EXIT_SUCCESS;
}