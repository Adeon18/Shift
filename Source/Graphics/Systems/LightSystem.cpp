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

        auto id = GUIDGenerator::GetInstance().Guid();
        m_pointLights[id] = LightEntry<PointLight>{
                .light = PointLight{.position=glm::vec4(position, 1.0f), .radiance=glm::vec4(radiance, 1.0f)},
                .bufferIndex = m_lightBuffer.lightCounts.y
        };

        m_lightBuffer.pointLights[m_lightBuffer.lightCounts.y++] = m_pointLights[id].light;

        // DANGER: STATIC
        m_meshSystem.AddInstance(MeshPass::Emission_Forward, Mobility::STATIC, m_pointLightModel, glm::scale(glm::translate(glm::mat4(1), position), glm::vec3(0.2)), m_pointLights[id].light.radiance);

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
        for (auto& [id, l]: m_directinalLights) {
            m_lightBuffer.directionalLights[l.bufferIndex] = l.light;
        }

        for (auto& [id, l]: m_pointLights) {
            m_lightBuffer.pointLights[l.bufferIndex] = l.light;
        }

        // TODO: This for now
        auto& b = m_bufferManager.GetUBO(m_lightBufferId, currentFrame);
        b.Fill(&m_lightBuffer, sizeof(m_lightBuffer));
    }
} // shift::gfx