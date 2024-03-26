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
            m_renderTargets[id][i] = std::make_unique<RenderTerget2D>(m_device, width, height, format);

            // MASSIVE TODO: THINK HOW TO CONNECT IT WITH FIF
            if (!IsReplaced) {
                m_UI.textureIdToDescriptorIdLUT[id][i] = m_descriptorManager.AllocateImGuiSet(ImGuiSetLayoutType::TEXTURE);
            } else {
                m_UI.textureIdToDescriptorIdLUT[id][i] = m_UI.textureIdToDescriptorIdLUT[prevId][i];
                //m_UI.textureIdToDescriptorIdLUT.erase(prevId);
            }
            auto& texSet = m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, m_UI.textureIdToDescriptorIdLUT[id][i]);
            texSet.UpdateImage(0, m_renderTargets[id][i]->GetView(), m_samplerManager.GetLinearSampler());
            texSet.ProcessUpdates();
        }

        return id;
    }

    RenderTerget2D &RenderTargetSystem::GetRTCurrentFrame(SGUID id, uint32_t currentFrame) {
        return *m_renderTargets[id][currentFrame];
    }

    RenderTerget2D &RenderTargetSystem::GetRTPrevFrame(SGUID id, uint32_t currentFrame) {
        uint32_t prevFrame = currentFrame - 1;
        prevFrame = (prevFrame == UINT32_MAX) ? gutil::SHIFT_MAX_FRAMES_IN_FLIGHT - 1: prevFrame;
        return *m_renderTargets[id][prevFrame];
    }

    RenderTerget2D &RenderTargetSystem::GetRTCurrentFrame(const std::string &name, uint32_t currentFrame) {
        return GetRTCurrentFrame(m_RTNameToId[name], currentFrame);
    }

    RenderTerget2D &RenderTargetSystem::GetRTPrevFrame(const std::string &name, uint32_t currentFrame) {
        return GetRTPrevFrame(m_RTNameToId[name], currentFrame);
    }

    bool RenderTargetSystem::IsValid(SGUID id) {
        return (m_renderTargets.find(id) != m_renderTargets.end());
    }

    SGUID RenderTargetSystem::IdByName(const std::string &name) {
        return m_RTNameToId[name];
    }

    void RenderTargetSystem::UI::Show(uint32_t currentFrame) {
        if (m_shown) {
            ImGui::Begin(m_name.c_str(), &m_shown);

            ImVec4 borderCol = ImGui::GetStyleColorVec4(ImGuiCol_Border);

            ImGui::SeparatorText("Texture Name");
            static char buff[512];
            ImGui::InputText("512 chars max", buff, IM_ARRAYSIZE(buff), 0);
            std::string bufStr{buff};

            ImGui::SeparatorText("Loaded Textures");
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

            ImGui::End();
        }
    }
} // shift::gfx
