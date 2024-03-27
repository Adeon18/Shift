//
// Created by otrush on 3/23/2024.
//

#include "RenderTargetSystem.hpp"

#include "Utility/UtilStandard.hpp"

namespace shift::gfx {
    RenderTargetSystem::RenderTargetSystem(const shift::gfx::Device &device,
                                           const shift::gfx::SamplerManager &samplerManager,
                                           shift::gfx::DescriptorManager &descriptorManager):
                                                m_device{device}, m_samplerManager{samplerManager}, m_descriptorManager{descriptorManager}
    {

    }

    SGUID RenderTargetSystem::CreateRenderTarget2D(uint32_t width, uint32_t height, VkFormat format, const std::string& name) {
        bool IsReplaced = (m_RTNameToId[name] != 0);
        SGUID prevId{};
        //! If the RT was created with such name, recreate it, used for window resize
        if (IsReplaced) {
            prevId = m_RTNameToId[name];
            for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
                m_renderTargets[prevId][i].reset();
            }
            m_renderTargets.erase(prevId);
            spdlog::info("Recreated RT: " + name);
        }

        auto id = GUIDGenerator::GetInstance().Guid();
        m_RTNameToId[name] = id;
        for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_renderTargets[id][i] = std::make_unique<ColorRenderTerget2D>(m_device, width, height, format, TextureType::Color);

            if (!IsReplaced) {
                m_UI.textureIdToDescriptorIdLUT[id][i] = m_descriptorManager.AllocateImGuiSet(ImGuiSetLayoutType::TEXTURE);
            } else {
                m_UI.textureIdToDescriptorIdLUT[id][i] = m_UI.textureIdToDescriptorIdLUT[prevId][i];
            }
            auto& texSet = m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, m_UI.textureIdToDescriptorIdLUT[id][i]);
            texSet.UpdateImage(0, m_renderTargets[id][i]->GetView(), m_samplerManager.GetLinearSampler());
            texSet.ProcessUpdates();
        }
        // Erase old id
        m_UI.textureIdToDescriptorIdLUT.erase(prevId);

        return id;
    }

    SGUID RenderTargetSystem::CreateDepthTarget2D(uint32_t width, uint32_t height, VkFormat format, const std::string& name) {
        bool IsReplaced = (m_DTNameToId[name] != 0);
        SGUID prevId{};
        //! If the RT was created with such name, recreate it, used for window resize
        if (IsReplaced) {
            prevId = m_DTNameToId[name];
            for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
                m_depthTargets[prevId][i].reset();
            }
            m_depthTargets.erase(prevId);
            spdlog::info("Recreated Depth Target: " + name);
        }

        auto id = GUIDGenerator::GetInstance().Guid();
        m_DTNameToId[name] = id;
        for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_depthTargets[id][i] = std::make_unique<DepthRenderTerget2D>(m_device, width, height, format, TextureType::Depth);

            if (!IsReplaced) {
                m_UI.textureIdToDescriptorIdLUT[id][i] = m_descriptorManager.AllocateImGuiSet(ImGuiSetLayoutType::TEXTURE);
            } else {
                m_UI.textureIdToDescriptorIdLUT[id][i] = m_UI.textureIdToDescriptorIdLUT[prevId][i];
            }
            auto& texSet = m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, m_UI.textureIdToDescriptorIdLUT[id][i]);
            texSet.UpdateImage(0, m_depthTargets[id][i]->GetView(), m_samplerManager.GetLinearSampler());
            texSet.ProcessUpdates();
        }
        // Erase old id
        m_UI.textureIdToDescriptorIdLUT.erase(prevId);

        return id;
    }

    ColorRenderTerget2D &RenderTargetSystem::GetColorRTCurrentFrame(SGUID id, uint32_t currentFrame) {
        return *m_renderTargets[id][currentFrame];
    }

    ColorRenderTerget2D &RenderTargetSystem::GetColorRTPrevFrame(SGUID id, uint32_t currentFrame) {
        uint32_t prevFrame = currentFrame - 1;
        prevFrame = (prevFrame == UINT32_MAX) ? gutil::SHIFT_MAX_FRAMES_IN_FLIGHT - 1: prevFrame;
        return *m_renderTargets[id][prevFrame];
    }

    ColorRenderTerget2D &RenderTargetSystem::GetColorRTCurrentFrame(const std::string &name, uint32_t currentFrame) {
        return GetColorRTCurrentFrame(m_RTNameToId[name], currentFrame);
    }

    ColorRenderTerget2D &RenderTargetSystem::GetColorRTPrevFrame(const std::string &name, uint32_t currentFrame) {
        return GetColorRTPrevFrame(m_RTNameToId[name], currentFrame);
    }

    bool RenderTargetSystem::IsValid(SGUID id) {
        return (m_renderTargets.find(id) != m_renderTargets.end());
    }

    SGUID RenderTargetSystem::IdByName(const std::string &name) {
        return m_RTNameToId[name];
    }

    DepthRenderTerget2D &RenderTargetSystem::GetDepthRTCurrentFrame(SGUID id, uint32_t currentFrame) {
        return *m_depthTargets[id][0];
    }

    DepthRenderTerget2D &RenderTargetSystem::GetDepthRTCurrentFrame(const std::string &name, uint32_t currentFrame) {
        return *m_depthTargets[m_DTNameToId[name]][0];
    }

    void RenderTargetSystem::UI::Show(uint32_t currentFrame) {
        if (m_shown) {
            ImGui::Begin(m_name.c_str(), &m_shown);

            ImVec4 borderCol = ImGui::GetStyleColorVec4(ImGuiCol_Border);

            ImGui::SeparatorText("Texture Name");
            static char buff[512];
            ImGui::InputText("512 chars max", buff, IM_ARRAYSIZE(buff), 0);
            std::string bufStr{buff};

            ImGui::SeparatorText("Render Targets");
            for (auto& [name, id]: m_system.m_RTNameToId) {
                if (id == 0) continue;
                if (util::StrToLower(name).find(util::StrToLower(bufStr)) == std::string::npos) {
                    continue;
                }

                ImGui::PushID(id);

                if (ImGui::CollapsingHeader(std::string{name + "##" + std::to_string(id)}.c_str())) {
                    glm::ivec2 texSize = {m_system.m_renderTargets[id][currentFrame]->GetWidth(), m_system.m_renderTargets[id][currentFrame]->GetHeight()};
                    float ratio = static_cast<float>(texSize.x) / static_cast<float>(texSize.y);

                    auto set = m_system.m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, textureIdToDescriptorIdLUT[id][currentFrame]).Get();

                    ImGui::Image(
                            set,
                            ImVec2(256 * ratio, 256),
                            ImVec2(0, 0),
                            ImVec2(1, 1),
                            ImVec4(1, 1, 1, 1),
                            borderCol
                    );

                    if (ImGui::TreeNode("Misc")) {
                        ImGui::LabelText(std::to_string(id).c_str(), "GUID");
                        std::string res = std::to_string(texSize.x) + "x" + std::to_string(texSize.y);
                        ImGui::LabelText(res.c_str(), "Texture Res");
                        ImGui::TreePop();
                    }
                }

                ImGui::PopID();
            }

            //! TODO: Can be made in fewer lines, code duplication
            ImGui::SeparatorText("Depth Targets");
            for (auto& [name, id]: m_system.m_DTNameToId) {
                if (id == 0) continue;
                if (util::StrToLower(name).find(util::StrToLower(bufStr)) == std::string::npos) {
                    continue;
                }

                ImGui::PushID(id);

                if (ImGui::CollapsingHeader(std::string{name + "##" + std::to_string(id)}.c_str())) {
                    glm::ivec2 texSize = {m_system.m_depthTargets[id][0]->GetWidth(), m_system.m_depthTargets[id][0]->GetHeight()};
                    float ratio = static_cast<float>(texSize.x) / static_cast<float>(texSize.y);

                    auto set = m_system.m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, textureIdToDescriptorIdLUT[id][0]).Get();

                    ImGui::Image(
                            set,
                            ImVec2(256 * ratio, 256),
                            ImVec2(0, 0),
                            ImVec2(1, 1),
                            ImVec4(1, 1, 1, 1),
                            borderCol
                    );

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
