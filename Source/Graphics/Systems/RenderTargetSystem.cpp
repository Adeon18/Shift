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
            m_renderTargets[prevId].reset();
            m_renderTargets.erase(prevId);
            spdlog::info("Recreated RT: " + name);
        }

        auto id = GUIDGenerator::GetInstance().Guid();
        m_RTNameToId[name] = id;
        m_renderTargets[id] = std::make_unique<ColorRenderTerget2D>(m_device, width, height, format, TextureType::Color);

        if (!IsReplaced) {
            m_UI.textureIdToDescriptorIdLUT[id] = m_descriptorManager.AllocateImGuiSet(ImGuiSetLayoutType::TEXTURE);
        } else {
            m_UI.textureIdToDescriptorIdLUT[id] = m_UI.textureIdToDescriptorIdLUT[prevId];
        }
        m_UI.textureUIScales[id] = 1.0f;
        auto& texSet = m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, m_UI.textureIdToDescriptorIdLUT[id]);
        texSet.UpdateImage(0, m_renderTargets[id]->GetView(), m_samplerManager.GetLinearSampler());
        texSet.ProcessUpdates();

        // Erase old id
        m_UI.textureIdToDescriptorIdLUT.erase(prevId);
        m_UI.textureUIScales.erase(prevId);

        return id;
    }

    // TODO: Code duplication
    SGUID RenderTargetSystem::CreateDepthTarget2D(uint32_t width, uint32_t height, VkFormat format, const std::string& name) {
        bool IsReplaced = (m_DTNameToId[name] != 0);
        SGUID prevId{};
        //! If the RT was created with such name, recreate it, used for window resize
        if (IsReplaced) {
            prevId = m_DTNameToId[name];
            m_depthTargets[prevId].reset();
            m_depthTargets.erase(prevId);
            spdlog::info("Recreated Depth Target: " + name);
        }

        auto id = GUIDGenerator::GetInstance().Guid();
        m_DTNameToId[name] = id;

        m_depthTargets[id] = std::make_unique<DepthRenderTerget2D>(m_device, width, height, format, TextureType::Depth);

        if (!IsReplaced) {
            m_UI.textureIdToDescriptorIdLUT[id] = m_descriptorManager.AllocateImGuiSet(ImGuiSetLayoutType::TEXTURE);
        } else {
            m_UI.textureIdToDescriptorIdLUT[id] = m_UI.textureIdToDescriptorIdLUT[prevId];
        }
        m_UI.textureUIScales[id] = 1.0f;
        auto& texSet = m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, m_UI.textureIdToDescriptorIdLUT[id]);
        texSet.UpdateImage(0, m_depthTargets[id]->GetView(), m_samplerManager.GetLinearSampler());
        texSet.ProcessUpdates();

        // Erase old id
        m_UI.textureIdToDescriptorIdLUT.erase(prevId);
        m_UI.textureUIScales.erase(prevId);

        return id;
    }

    ColorRenderTerget2D &RenderTargetSystem::GetColorRT(SGUID id) {
        return *m_renderTargets[id];
    }

    ColorRenderTerget2D &RenderTargetSystem::GetColorRT(const std::string &name) {
        return GetColorRT(m_RTNameToId[name]);
    }

    bool RenderTargetSystem::IsValid(SGUID id) {
        return (m_renderTargets.find(id) != m_renderTargets.end());
    }

    SGUID RenderTargetSystem::IdByName(const std::string &name) {
        return m_RTNameToId[name];
    }

    DepthRenderTerget2D &RenderTargetSystem::GetDepthRT(SGUID id) {
        return *m_depthTargets[id];
    }

    DepthRenderTerget2D &RenderTargetSystem::GetDepthRT(const std::string &name) {
        return *m_depthTargets[m_DTNameToId[name]];
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

                ImGui::PushID(static_cast<int>(id));

                if (ImGui::CollapsingHeader(std::string{name + "##" + std::to_string(id)}.c_str())) {
                    glm::ivec2 texSize = {m_system.m_renderTargets[id]->GetWidth(), m_system.m_renderTargets[id]->GetHeight()};
                    float ratio = static_cast<float>(texSize.x) / static_cast<float>(texSize.y);

                    float &texViewScale = textureUIScales[id];
                    ImGui::DragFloat("UI Texture Scale", &texViewScale, 0.05f, 0.05f, 16.0f);

                    auto set = m_system.m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, textureIdToDescriptorIdLUT[id]).Get();

                    ImGui::Image(
                            set,
                            ImVec2(DEFAULT_UI_TEX_SIZE * ratio * texViewScale, DEFAULT_UI_TEX_SIZE * texViewScale),
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

                ImGui::PushID(static_cast<int>(id));

                if (ImGui::CollapsingHeader(std::string{name + "##" + std::to_string(id)}.c_str())) {
                    glm::ivec2 texSize = {m_system.m_depthTargets[id]->GetWidth(), m_system.m_depthTargets[id]->GetHeight()};
                    float ratio = static_cast<float>(texSize.x) / static_cast<float>(texSize.y);

                    auto set = m_system.m_descriptorManager.GetImGuiSet(ImGuiSetLayoutType::TEXTURE, textureIdToDescriptorIdLUT[id]).Get();

                    float &texViewScale = textureUIScales[id];
                    ImGui::DragFloat("UI Texture Scale", &texViewScale, 0.05f, 0.05f, 16.0f);

                    ImGui::Image(
                            set,
                            ImVec2(DEFAULT_UI_TEX_SIZE * ratio * texViewScale, DEFAULT_UI_TEX_SIZE * texViewScale),
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
