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
        auto id = GUIDGenerator::GetInstance().Guid();
        m_pointLights[id] = LightEntry<PointLight>{
                .light = PointLight{.position=glm::vec4(position, 1.0f), .radiance=glm::vec4(radiance, 1.0f)},
                .insId = lightVisInsId,
                .bufferIndex = m_lightBuffer.lightCounts.y
        };

        m_lightBuffer.pointLights[m_lightBuffer.lightCounts.y++] = m_pointLights[id].light;

        spdlog::info("Added Point Light, number: " + std::to_string(m_lightBuffer.lightCounts.y));
        return id;
    }

    SGUID LightSystem::AddDirectionalLight(const glm::vec3 direction, const glm::vec3 radiance) {
        if (m_lightBuffer.lightCounts.x == cfg::DIRECTIONAL_LIGHT_MAX_COUNT) {
            spdlog::warn("Directional Light limit reached on scene! Max count: " + std::to_string(cfg::DIRECTIONAL_LIGHT_MAX_COUNT));
            return 0;
        }
        auto id = GUIDGenerator::GetInstance().Guid();
        m_directinalLights[id] = LightEntry<DirectionalLight>{
                .light = DirectionalLight{.direction=glm::vec4(glm::normalize(direction), 1.0f), .radiance=glm::vec4(radiance, 1.0f)},
                .bufferIndex = m_lightBuffer.lightCounts.x
        };

        m_lightBuffer.directionalLights[m_lightBuffer.lightCounts.x++] = m_directinalLights[id].light;

        spdlog::info("Added Directional Light, number: " + std::to_string(m_lightBuffer.lightCounts.x));
        return id;
    }

    void LightSystem::UpdateAllLights(uint32_t currentFrame) {
        static glm::vec4 pos = glm::vec4(0.0f);
        pos.x -= 0.01f;
        if (pos.x < -3.0f) {
            pos.x = 0.0f;
        }
        m_pointLights.begin()->second.light.position = pos;
        std::cout << m_pointLights.begin()->second.light.position.y << std::endl;
        m_meshSystem.SetDynamicInstanceWorldPosition(MeshPass::Emission_Forward, m_pointLights.begin()->second.insId, m_pointLights.begin()->second.light.position);

        for (auto& [id, l]: m_directinalLights) {
            m_lightBuffer.directionalLights[l.bufferIndex] = l.light;
        }

        for (auto& [id, l]: m_pointLights) {
            m_lightBuffer.pointLights[l.bufferIndex] = l.light;
        }

        // TODO: This for now: updates everything
        auto& b = m_bufferManager.GetUBO(m_lightBufferId, currentFrame);
        b.Fill(&m_lightBuffer, sizeof(m_lightBuffer));
    }
} // shift::gfx