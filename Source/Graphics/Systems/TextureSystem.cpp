#include "TextureSystem.hpp"

#include <array>

#include <stb_image.h>
#include <spdlog/spdlog.h>

#include "Graphics/Abstraction/Buffers/BasicBuffers.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace shift::gfx {
    TextureSystem::TextureSystem(const shift::gfx::Device &device, shift::gfx::CommandPool &graphicsPool,
                                 shift::gfx::CommandPool &transferPool): m_device{device}, m_gfxPool{graphicsPool},
                                                                         m_transPool{transferPool}
    {
        CreateDefaultTextures();
    }

    SGUID TextureSystem::LoadTexture(const std::string &path, VkFormat format, std::string name) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 bytes per pixel, MAYBE TODO?

        if (!pixels) {
            spdlog::warn("Failed to read image!");
            return SGUID{0};
        }

        shift::gfx::StagingBuffer stagingBuff{m_device, imageSize};
        memcpy(stagingBuff.GetMappedBuffer(), pixels, static_cast<size_t>(imageSize));
        stbi_image_free(pixels);

        SGUID imageGUID = GUIDGenerator::GetInstance().Guid();
        m_textureIdByName[(name.empty()) ? path: name] = imageGUID;
        m_textures[imageGUID] = std::make_unique<shift::gfx::Texture2D>(m_device, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        auto& bufferCopy = m_transPool.RequestCommandBuffer();
        bufferCopy.TransferImageLayout(
                m_textures[imageGUID]->GetImage(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        bufferCopy.CopyBufferToImage(stagingBuff.Get(), m_textures[imageGUID]->GetImage(), m_textures[imageGUID]->GetWidth(), m_textures[imageGUID]->GetHeight());
        bufferCopy.EndCommandBuffer();
        bufferCopy.SubmitAndWait();

        auto& bufferFinal = m_gfxPool.RequestCommandBuffer();

        bufferFinal.TransferImageLayout(
                m_textures[imageGUID]->GetImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        bufferFinal.EndCommandBuffer();
        bufferFinal.SubmitAndWait();

        return imageGUID;
    }

    TextureBase *TextureSystem::GetTexture(SGUID guid) {
        return m_textures[guid].get();
    }

    void TextureSystem::CreateDefaultTextures() {
        CreateDefaultColorTexture({255, 255, 255, 255}, "WHITE");
        CreateDefaultColorTexture({0, 0, 0, 255},       "BLACK");
        CreateDefaultColorTexture({255, 0, 0, 255},     "RED");
        CreateDefaultColorTexture({0, 255, 0, 255},     "GREEN");
        CreateDefaultColorTexture({0, 0, 255, 255},     "BLUE");
        CreateDefaultColorTexture({128, 128, 128, 255}, "GRAY");
    }

    SGUID TextureSystem::CreateDefaultColorTexture(const std::array<uint8_t, 4>& color, std::string name) {
        const uint32_t size = sizeof(color);

        shift::gfx::StagingBuffer stagingBuff{m_device, size};
        memcpy(stagingBuff.GetMappedBuffer(), color.data(), static_cast<size_t>(size));

        SGUID imageGUID = GUIDGenerator::GetInstance().Guid();
        m_textureIdByName[name] = imageGUID;
        m_textures[imageGUID] = std::make_unique<shift::gfx::Texture2D>(m_device, 1, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        auto& bufferCopy = m_transPool.RequestCommandBuffer();
        bufferCopy.TransferImageLayout(
                m_textures[imageGUID]->GetImage(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        bufferCopy.CopyBufferToImage(stagingBuff.Get(), m_textures[imageGUID]->GetImage(), m_textures[imageGUID]->GetWidth(), m_textures[imageGUID]->GetHeight());
        bufferCopy.EndCommandBuffer();
        bufferCopy.SubmitAndWait();

        auto& bufferFinal = m_gfxPool.RequestCommandBuffer();

        bufferFinal.TransferImageLayout(
                m_textures[imageGUID]->GetImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        bufferFinal.EndCommandBuffer();
        bufferFinal.SubmitAndWait();

        return imageGUID;
    }

    TextureBase *TextureSystem::GetTexture(std::string name) {
        return m_textures[m_textureIdByName[name]].get();
    }
} // shift::gfx

