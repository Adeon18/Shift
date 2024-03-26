//
// Created by otrush on 3/18/2024.
//

#ifndef SHIFT_LIGHTSYSTEM_HPP
#define SHIFT_LIGHTSYSTEM_HPP

#include "MeshSystem.hpp"

#include "Graphics/Objects/LightTypes.hpp"
#include "Graphics/Abstraction/Descriptors/UBOStructs.hpp"

#include "Graphics/UI/UIWindowComponent.hpp"
#include "Graphics/UI/UIManager.hpp"

namespace shift::gfx {
    //! A system that manages lights, addition, deletion, updates
    class LightSystem {
        class UI: public ui::UIToolComponent {
        public:
            explicit UI(std::string name, LightSystem& system): ui::UIToolComponent(std::move(name)), m_system{system} {
                ui::UIManager::GetInstance().RegisterToolComponent(this);
            }

            virtual void Item() override { ui::UIToolComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;
        private:
            LightSystem& m_system;
        };
    public:
        // TODO: remake the model vis based on name
        LightSystem(DescriptorManager& dMan, BufferManager& bMan, MeshSystem& mSys, SGUID pointLightModel): m_descriptorManager{dMan}, m_bufferManager{bMan}, m_meshSystem{mSys}, m_pointLightModel{pointLightModel} {
            m_lightBuffer.lightCounts = glm::ivec2(0, 0);
            m_lightBufferId = GUIDGenerator::GetInstance().Guid();
            VkDeviceSize size = sizeof(LightsPerFrame);
            m_bufferManager.AllocateUBO(m_lightBufferId, size);

            for (uint32_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
                auto &perFrameSet = m_descriptorManager.GetPerFrameSet(i);
                auto &buff = m_bufferManager.GetUBO(m_lightBufferId, i);
                perFrameSet.UpdateUBO(1, buff.Get(), 0, buff.GetSize());
                perFrameSet.ProcessUpdates();
            }
        }

        SGUID AddPointLight(const glm::vec3 position, const glm::vec3 radiance);
        SGUID AddDirectionalLight(const glm::vec3 direction, const glm::vec3 radiance);

        void UpdateAllLights(uint32_t currentFrame);

        ~LightSystem() = default;

        LightSystem() = delete;
        LightSystem(const LightSystem&) = delete;
        LightSystem& operator=(const LightSystem&) = delete;
    private:
        UI m_UI{"Light System", *this};

        template<typename T>
        struct LightEntry {
            T light;
            SGUID lightInsId;
            SGUID meshInsId = 0;
            uint32_t bufferIndex;
        };

        DescriptorManager& m_descriptorManager;
        BufferManager& m_bufferManager;
        MeshSystem& m_meshSystem;

        std::vector<LightEntry<DirectionalLight>> m_directinalLights;
        std::vector<LightEntry<PointLight>> m_pointLights;

        LightsPerFrame m_lightBuffer;
        SGUID m_lightBufferId;

        SGUID m_pointLightModel;
    };
} // shift::gfx

#endif //SHIFT_LIGHTSYSTEM_HPP
