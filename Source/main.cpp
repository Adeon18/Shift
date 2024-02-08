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
#include "Graphics/Abstraction/Device/WindowSurface.hpp"
#include "Graphics/Abstraction/Device/Instance.hpp"
#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Device/Swapchain.hpp"
#include "Graphics/Abstraction/Synchronization/Fence.hpp"
#include "Graphics/Abstraction/Synchronization/Semaphore.hpp"
#include "Graphics/Abstraction/Commands/CommandPool.hpp"
#include "Graphics/Abstraction/Commands/CommandBuffer.hpp"
#include "Graphics/Abstraction/RenderPass/RenderPass.hpp"
#include "Graphics/Abstraction/Pipeline/Shader.hpp"
#include "Graphics/Abstraction/Pipeline/Pipeline.hpp"
#include "Graphics/Abstraction/Descriptors/DescriptorManagement.hpp"

#include <spdlog/spdlog.h>

class HelloTriangleApplication {
    static constexpr uint32_t WINDOW_WIDTH = 800;
    static constexpr uint32_t WINDOW_HEIGHT = 600;

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
        if (!createRenderPass()) { return false; }
        createDescriptorSetLayout();
        createFramebuffers();
        createCommandPools();

        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        if (!createDescriptorPool()) {return false;}
        createDescriptorSets();

        if (!createGraphicsPipeline()) {return false;}

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
//        m_descriptorSets.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
//
//        for (auto &st: m_descriptorSets)
//
//        VkDescriptorSetLayoutBinding uboLayoutBinding{};
//        uboLayoutBinding.binding = 0;
//        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        uboLayoutBinding.descriptorCount = 1;   // It is possible for the shader to represent an array of UBOs
//        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   // Shader type, can specify all
//        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
//
//        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
//        samplerLayoutBinding.binding = 1;
//        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        samplerLayoutBinding.descriptorCount = 1;   // It is possible for the shader to represent an array of UBOs
//        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;   // Shader type, can specify all
//        samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional
//
//        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
//        VkDescriptorSetLayoutCreateInfo layoutInfo{};
//        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//        layoutInfo.pBindings = bindings.data();
//
//        if (vkCreateDescriptorSetLayout(m_device->Get(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
//            throw std::runtime_error("Failed to create descriptor set layout!");
//        }
    }

    bool createGraphicsPipeline() {
        sft::gfx::Shader vert{*m_device, sft::util::GetShiftRoot() + "Shaders/shader.vert.spv", sft::gfx::Shader::Type::Vertex};
        if (!vert.CreateStage()) {return false;}

        sft::gfx::Shader frag{*m_device, sft::util::GetShiftRoot() + "Shaders/shader.frag.spv", sft::gfx::Shader::Type::Fragment};
        if (!frag.CreateStage()) {return false;}

        m_pipeline = std::make_unique<sft::gfx::Pipeline>(*m_device);

        m_pipeline->AddShaderStage(vert);
        m_pipeline->AddShaderStage(frag);

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        m_pipeline->SetInputStateInfo(sft::info::CreateInputStateInfo(attributeDescriptions, {&bindingDescription, 1}),VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        m_pipeline->SetViewPortState();

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        m_pipeline->SetDynamicState(dynamicStates);

        m_pipeline->SetRasterizerInfo(sft::info::CreateRasterStateInfo());

        m_pipeline->SetMultisampleInfo(sft::info::CreateMultisampleStateInfo());

        auto blendState = sft::info::CreateBlendAttachmentState();
        m_pipeline->SetBlendAttachment(blendState);
        m_pipeline->SetBlendState(sft::info::CreateBlendStateInfo(blendState));

        m_pipeline->BuildLayout({m_descriptorSets[m_currentFrame]->GetLayoutPtr(), 1});

        return m_pipeline->Build(*m_renderPass);
    }

    bool createRenderPass() {
        m_renderPass = std::make_unique<sft::gfx::RenderPass>(*m_device);

        sft::gfx::Attachment att{m_swapchain->GetFormat(), sft::gfx::Attachment::Type::Swapchain, 0};

        sft::gfx::Subpass sub;
        sub.AddDependency(att);
        sub.BuildDescription();

        m_renderPass->AddAttachment(att);
        m_renderPass->AddSubpass(sub);

        return m_renderPass->BuildPass();
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
            framebufferInfo.renderPass = m_renderPass->Get();
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
        m_graphicsPool = std::make_unique<sft::gfx::CommandPool>(*m_device, sft::gfx::POOL_TYPE::GRAPHICS);
        m_transferPool = std::make_unique<sft::gfx::CommandPool>(*m_device, sft::gfx::POOL_TYPE::TRANSFER);
    }

    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        std::string a = sft::util::GetShiftRoot() + "Assets/skeleton.png";
        stbi_uc* pixels = stbi_load(a.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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

        auto& buffer = m_transferPool->RequestCommandBuffer();
        buffer.TransferImageLayout(
                m_textureImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT
                );

        buffer.CopyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        buffer.EndCommandBuffer();
        buffer.SubmitAndWait();

        auto& buffer2 = m_graphicsPool->RequestCommandBuffer();

        buffer2.TransferImageLayout(
                m_textureImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        buffer2.EndCommandBuffer();
        buffer2.SubmitAndWait();

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

        auto& buffer = m_transferPool->RequestCommandBuffer();
        buffer.CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
        buffer.EndCommandBuffer();
        buffer.SubmitAndWait();

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

        auto& buffer = m_transferPool->RequestCommandBuffer();
        buffer.CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
        buffer.EndCommandBuffer();
        buffer.SubmitAndWait();

        // Destroy staging buffer
        vkDestroyBuffer(m_device->Get(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->Get(), stagingBufferMemory, nullptr);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(PerFrame);

        m_uniformBuffers.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMemory.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMapped.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            // The technique is called PERSISTENT MAPPING, the buffer is mapped during the entire duration and we do not map
            // it every frame, it is faster this way
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);

            vkMapMemory(m_device->Get(), m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);
        }
    }

    bool createDescriptorPool() {
        m_descriptorPool = std::make_unique<sft::gfx::DescriptorPool>(*m_device);
        m_descriptorPool->AddUBOSize(10);
        m_descriptorPool->AddSamplerSize(10);
        m_descriptorPool->SetMaxSets(10);
        return m_descriptorPool->Build();
    }

    void createDescriptorSets() {
        m_descriptorSets.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_descriptorSets[i] = std::make_unique<sft::gfx::DescriptorSet>(*m_device);
            m_descriptorSets[i]->AddUBO<PerFrame>(m_uniformBuffers[i], 0, 0, VK_SHADER_STAGE_VERTEX_BIT);
            m_descriptorSets[i]->AddImage(m_textureImageView, m_textureSampler, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
            m_descriptorSets[i]->Build(m_descriptorPool->Get());
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

    void recordCommandBuffer(const sft::gfx::CommandBuffer& cmdBuf, uint32_t imageIndex) {

        //! Begin a render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass->Get();
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchain->GetExtent();

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        cmdBuf.BeginRenderPass(renderPassInfo);

        cmdBuf.BindPipeline(m_pipeline->Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);

        std::array<VkBuffer, 1> vertexBuffers{ m_vertexBuffer };
        std::array<VkDeviceSize, 1> offsets{ 0 };
        cmdBuf.BindVertexBuffers(vertexBuffers, offsets, 0);
        cmdBuf.BindIndexBuffer(m_indexBuffer, 0);

        // Since the viewport and scissor are dynamic, we must set them here
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmdBuf.SetViewPort(viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchain->GetExtent();
        cmdBuf.SetScissor(scissor);

        // Bind the descriptor sets
        // INFO: The sets are not unique to pipelines, so we need to specify whether to bind it to compute pipeline or the graphics one
        std::array<VkDescriptorSet, 1> sets{ m_descriptorSets[m_currentFrame]->Get() };
        cmdBuf.BindDescriptorSets(sets, {}, m_pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

        // DRAW THE FUCKING TRIANGLE
        cmdBuf.DrawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        cmdBuf.EndRenderPass();

        cmdBuf.EndCommandBuffer();
    }

    void createSyncObjects() {
        m_imageAvailableSemaphores.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
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
        auto& buff = m_graphicsPool->RequestCommandBuffer(sft::gfx::BUFFER_TYPE::FLIGHT, m_currentFrame);

        /// Aquire availible swapchain image index
        bool changed = false;
        uint32_t imageIndex = m_swapchain->AquireNextImageIndex(*m_imageAvailableSemaphores[m_currentFrame], &changed);
        if (imageIndex == UINT32_MAX) return false;
        if (changed) {
            if (!recreateSwapChain()) return false;
            return true;
        }

        recordCommandBuffer(buff, imageIndex);

        updateUniformBuffer(m_currentFrame);

        std::array<VkSemaphore, 1> waitSem{ m_imageAvailableSemaphores[m_currentFrame]->Get() };
        std::array<VkSemaphore, 1> sigSem{ m_renderFinishedSemaphores[m_currentFrame]->Get() };
        std::array<VkCommandBuffer, 1> cmdBuf{ buff.Get() };
        std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        buff.Submit(sft::info::CreateSubmitInfo(
                    waitSem,
                    sigSem,
                    cmdBuf,
                    waitStages.data()
                ));

        bool isOld = false;
        bool success = m_swapchain->Present(*m_renderFinishedSemaphores[m_currentFrame], imageIndex, &isOld);
        if (!success) {return false;}

        if (isOld || m_winPtr->ShouldProcessResize()) {
            m_winPtr->ProcessResize();
            if (!recreateSwapChain()) return false;
        }

        // Update the current frame
        m_currentFrame = (m_currentFrame + 1) % sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT;

        return true;
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        PerFrame pf{};
        pf.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //pf.model = glm::mat4(1.0f);

        pf.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //pf.view = glm::mat4(1.0f);

        pf.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_swapchain->GetExtent().width) / static_cast<float>(m_swapchain->GetExtent().height), 0.1f, 100.0f);
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

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(m_device->Get(), m_uniformBuffers[i], nullptr);
            vkFreeMemory(m_device->Get(), m_uniformBuffersMemory[i], nullptr);
        }

        m_descriptorPool.reset();

        vkDestroyBuffer(m_device->Get(), m_vertexBuffer, nullptr);
        vkFreeMemory(m_device->Get(), m_vertexBufferMemory, nullptr);

        vkDestroyBuffer(m_device->Get(), m_indexBuffer, nullptr);
        vkFreeMemory(m_device->Get(), m_indexBufferMemory, nullptr);

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_descriptorSets[i].reset();
            m_imageAvailableSemaphores[i].reset();
            m_renderFinishedSemaphores[i].reset();
            m_inFlightFences[i].reset();
        }
        m_graphicsPool.reset();
        m_transferPool.reset();

        m_pipeline.reset();

        m_renderPass.reset();

        m_surfacePtr.reset();
        m_device.reset();
    }
private:
    std::unique_ptr<sft::gfx::DescriptorPool> m_descriptorPool;
    std::vector<std::unique_ptr<sft::gfx::DescriptorSet>> m_descriptorSets;

    std::unique_ptr<sft::ShiftWindow> m_winPtr;
    std::unique_ptr<sft::gfx::Device> m_device;
    std::unique_ptr<sft::gfx::WindowSurface> m_surfacePtr;
    std::unique_ptr<sft::gfx::Instance> m_instance;
    std::unique_ptr<sft::gfx::Swapchain> m_swapchain;

    std::unique_ptr<sft::gfx::CommandPool> m_graphicsPool;
    std::unique_ptr<sft::gfx::CommandPool> m_transferPool;

    std::unique_ptr<sft::gfx::RenderPass> m_renderPass;

    std::unique_ptr<sft::gfx::Pipeline> m_pipeline;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    //std::vector<VkCommandBuffer> m_commandBuffersGraphics;

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