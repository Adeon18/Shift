#include "TextureSystem.hpp"

#include <array>

#include <stb_image.h>
#include <spdlog/spdlog.h>

#include "Graphics/Abstraction/Buffers/BasicBuffers.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"
#include "Utility/UtilStandard.hpp"

namespace shift::gfx {
    TextureSystem::TextureSystem(const shift::gfx::Device &device,
                                 const SamplerManager& samplerManager,
                                 shift::gfx::CommandPool &graphicsPool,
                                 shift::gfx::CommandPool &transferPool,
                                 DescriptorManager& descriptorManager):
                                    m_device{device},
                                    m_samplerManager{samplerManager},
                                    m_gfxPool{graphicsPool},
                                    m_transPool{transferPool},
                                    m_descriptorManager{descriptorManager}
    {
        CreateDefaultTextures();
    }

    SGUID TextureSystem::LoadTexture(const std::string &path, VkFormat format, std::string name) {
        if (m_textureIdByName[path] != 0) {
            return m_textureIdByName[path];
        }

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

        m_UI.textureIdToDescriptorIdLUT[imageGUID] = m_descriptorManager.AllocateImGuiSet(ImGuiSetLayoutType::TEXTURE);
        auto& texSet = m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, m_UI.textureIdToDescriptorIdLUT[imageGUID]);
        texSet.UpdateImage(0, m_textures[imageGUID]->GetView(), m_samplerManager.GetLinearSampler());
        texSet.ProcessUpdates();

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

        // UI
        m_UI.textureIdToDescriptorIdLUT[imageGUID] = m_descriptorManager.AllocateImGuiSet(ImGuiSetLayoutType::TEXTURE);
        auto& texSet = m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, m_UI.textureIdToDescriptorIdLUT[imageGUID]);
        texSet.UpdateImage(0, m_textures[imageGUID]->GetView(), m_samplerManager.GetLinearSampler());
        texSet.ProcessUpdates();

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

    void TextureSystem::UI::Show() {
        if (m_shown) {
            ImGui::Begin(m_name.c_str(), &m_shown);

            ImGuiIO& io = ImGui::GetIO();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 winSize = ImGui::GetWindowSize();
            ImVec4 borderCol = ImGui::GetStyleColorVec4(ImGuiCol_Border);

            ImGui::SeparatorText("Texture Name");
            static char buff[512];
            ImGui::InputText("512 chars max", buff, IM_ARRAYSIZE(buff), 0);
            std::string bufStr{buff};

            ImGui::SeparatorText("Loaded Textures");
            for (auto& [name, id]: m_system.m_textureIdByName) {
                if (id == 0) continue;
                if (util::StrToLower(name).find(util::StrToLower(bufStr)) == std::string::npos) {
                    continue;
                }

                ImGui::PushID(id);

                if (ImGui::CollapsingHeader(std::string{name + "##" + std::to_string(id)}.c_str())) {
                    glm::ivec2 texSize = {m_system.m_textures[id]->GetWidth(), m_system.m_textures[id]->GetHeight()};

                    auto set = m_system.m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, textureIdToDescriptorIdLUT[id]).Get();

                    //ImVec2 texSizeIm = ImVec2{static_cast<float>((winSize.x - 256) * 0.5f), static_cast<float>((winSize.y - 256) * 0.5f)};
                    //ImGui::SetCursorPos(texSizeIm);
                    ImGui::Image(
                            set,
                            ImVec2(256, 256),
                            ImVec2(0, 0),
                            ImVec2(1, 1),
                            ImVec4(1, 1, 1, 1),
                            borderCol
                            );

//                    if (ImGui::BeginItemTooltip())
//                    {
//                        float region_sz = 32.0f;
//                        float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
//                        float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
//                        float zoom = 4.0f;
//                        if (region_x < 0.0f) { region_x = 0.0f; }
//                        else if (region_x > size.x - region_sz) { region_x = size.x - region_sz; }
//                        if (region_y < 0.0f) { region_y = 0.0f; }
//                        else if (region_y > size.y - region_sz) { region_y = size.y - region_sz; }
//                        ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
//                        ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
//                        ImVec2 uv0 = ImVec2((region_x) / size.x, (region_y) / size.y);
//                        ImVec2 uv1 = ImVec2((region_x + region_sz) / size.x, (region_y + region_sz) / size.y);
//                        ImGui::Image(set, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImVec4(1, 1, 1, 1), borderCol);
//                        ImGui::EndTooltip();
//                    }

                    if (ImGui::TreeNode("Misc")) {
                        ImGui::LabelText(std::to_string(id).c_str(), "GUID");
                        std::string res = std::to_string(texSize.x) + "x" + std::to_string(texSize.y);
                        ImGui::LabelText(res.c_str(), "Texture Res");
                        ImGui::TreePop();
                    }
                }

                ImGui::PopID();
            }

            ImGui::End();
        }
    }
} // shift::gfx

