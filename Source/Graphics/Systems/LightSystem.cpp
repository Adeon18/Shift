//
// Created by otrush on 3/18/2024.
//

#include "LightSystem.hpp"

namespace shift::gfx {
    SGUID LightSystem::AddPointLight(const glm::vec3 position, const glm::vec3 radiance) {
        if (m_lightBuffer.lightCounts.y == cfg::POINT_LIGHT_MAX_COUNT) {
            spdlog::warn("Point Light limit reached on scene! Max count: " + std::to_string(cfg::POINT_LIGHT_MAX_COUNT));
            return 0;
        }

        auto lightVisInsId = m_meshSystem.AddInstance(MeshPass::Emission_Forward, Mobility::MOVABLE, m_pointLightModel, glm::scale(glm::translate(glm::mat4(1), position), glm::vec3(0.2)), glm::vec4(radiance, 1.0f));
        m_pointLights.push_back(LightEntry<PointLight>{
                .light = PointLight{.position=glm::vec4(position, 1.0f), .radiance=glm::vec4(radiance, 1.0f)},
                .lightInsId = GUIDGenerator::GetInstance().Guid(),
                .meshInsId = lightVisInsId,
                .bufferIndex = m_lightBuffer.lightCounts.y
        });

        m_lightBuffer.pointLights[m_lightBuffer.lightCounts.y++] = m_pointLights.back().light;

        spdlog::info("Added Point Light, number: " + std::to_string(m_lightBuffer.lightCounts.y));
        return m_pointLights.back().lightInsId;
    }

    SGUID LightSystem::AddDirectionalLight(const glm::vec3 direction, const glm::vec3 radiance) {
        if (m_lightBuffer.lightCounts.x == cfg::DIRECTIONAL_LIGHT_MAX_COUNT) {
            spdlog::warn("Directional Light limit reached on scene! Max count: " + std::to_string(cfg::DIRECTIONAL_LIGHT_MAX_COUNT));
            return 0;
        }
        m_directinalLights.push_back(LightEntry<DirectionalLight>{
                .light = DirectionalLight{.direction=glm::vec4(glm::normalize(direction), 1.0f), .radiance=glm::vec4(radiance, 1.0f)},
                .lightInsId = GUIDGenerator::GetInstance().Guid(),
                .bufferIndex = m_lightBuffer.lightCounts.x
        });

        m_lightBuffer.directionalLights[m_lightBuffer.lightCounts.x++] = m_directinalLights.back().light;

        spdlog::info("Added Directional Light, number: " + std::to_string(m_lightBuffer.lightCounts.x));
        return m_directinalLights.back().lightInsId;
    }

    void LightSystem::UpdateAllLights(uint32_t currentFrame) {
        for (auto& lightEntry: m_directinalLights) {
            /// In case direction was changed via imgui
            lightEntry.light.direction = glm::normalize(lightEntry.light.direction);
            m_lightBuffer.directionalLights[lightEntry.bufferIndex] = lightEntry.light;
        }

        for (auto& lightEntry: m_pointLights) {
            m_lightBuffer.pointLights[lightEntry.bufferIndex] = lightEntry.light;
            m_meshSystem.SetDynamicInstanceWorldPosition(MeshPass::Emission_Forward, lightEntry.meshInsId, lightEntry.light.position);
            m_meshSystem.SetEmissionPassInstanceColor(lightEntry.meshInsId, lightEntry.light.radiance);
        }

        // TODO: This for now: updates everything
        auto& b = m_bufferManager.GetUBO(m_lightBufferId, currentFrame);
        b.Fill(&m_lightBuffer, sizeof(m_lightBuffer));
    }

    void LightSystem::UI::Show() {
        if (m_shown) {
            ImGui::Begin(m_name.c_str(), &m_shown);
            if (ImGui::CollapsingHeader("Directional Lights")) {
                for (auto& lE: m_system.m_directinalLights) {
                    ImGui::PushID(lE.bufferIndex);
                    ImGui::SeparatorText(std::string{"Light " + std::to_string(lE.bufferIndex)}.c_str());

                    ImGui::DragFloat3("Direction Vector", glm::value_ptr(lE.light.direction), 0.01f, -1.0f, 1.0f);
                    ImGui::ColorEdit3("Radiance", glm::value_ptr(lE.light.radiance));

                    if (ImGui::TreeNode("Misc")) {
                        ImGui::LabelText(std::to_string(lE.lightInsId).c_str(), "GUID");

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            if (ImGui::CollapsingHeader("Point Lights")) {
                for (auto& lE: m_system.m_pointLights) {
                    ImGui::PushID(lE.bufferIndex);
                    ImGui::SeparatorText(std::string{"Light " + std::to_string(lE.bufferIndex)}.c_str());

                    ImGui::DragFloat3("Position", glm::value_ptr(lE.light.position));
                    // TODO: What the fuck
                    ImGui::ColorEdit3(std::string{"Radiance##" + std::to_string(lE.bufferIndex)}.c_str(), glm::value_ptr(lE.light.radiance));

                    if (ImGui::TreeNode("Misc")) {
                        ImGui::LabelText(std::to_string(lE.lightInsId).c_str(), "GUID");
                        ImGui::LabelText(std::to_string(lE.meshInsId).c_str(), "Mesh GUID");

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::End();
        }
    }
} // shift::gfx