//
// Created by otrush on 2/27/2024.
//

#ifndef SHIFT_RENDERER_HPP
#define SHIFT_RENDERER_HPP

#include <glm/glm.hpp>

#include "ShiftContextData.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Device/Instance.hpp"
#include "Graphics/Abstraction/Device/Swapchain.hpp"
#include "Graphics/Abstraction/Commands/CommandPool.hpp"
#include "Graphics/Abstraction/Descriptors/DescriptorManager.hpp"
#include "Graphics/Abstraction/Pipeline/Pipeline.hpp"
#include "Graphics/Abstraction/Descriptors/BufferManager.hpp"
#include "Graphics/Abstraction/Images/SamplerManager.hpp"
#include "Graphics/Systems/TextureSystem.hpp"
#include "Graphics/Systems/ModelSystem.hpp"
#include "Graphics/Systems/RenderStage.hpp"
#include "Graphics/Systems/GeometrySystem.hpp"
#include "Graphics/Systems/LightSystem.hpp"
#include "Graphics/Systems/RenderTargetSystem.hpp"
#include "Graphics/Systems/PostProcessSystem.hpp"
#include "Graphics/UI/UIManager.hpp"

#include "Input/Controllers/Camera/FlyingCameraController.hpp"

namespace shift::gfx {
    //! A struct with data that can change per-frame
    struct EngineData {
        /// Camera controller
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::vec3 camPosition;
        glm::vec3 camDirection;
        glm::vec3 camRight;
        glm::vec3 camUp;
        /// Window data
        uint32_t winWidth;
        uint32_t winHeight;
        float oneDivWinWidth;
        float oneDivWinHeight;
        /// Timer data
        float dt;
        float fps;
        float secondsSinceStart;
    };

    class Renderer {

    public:
        Renderer(ShiftWindow& window, std::shared_ptr<ctrl::FlyingCameraController> controller): m_window{window}, m_controller(controller) {

        }

        //! Init the renderer and all child elements
        bool Init();

        // TODO: hardcoded
        bool LoadScene();

        //! Render entire frame
        bool RenderFrame(const EngineData& engineData);

        //! Cleanup unused resources
        void Cleanup();
    private:
        void CreateDescriptors();
        void CreateSyncPrimitives();

        [[nodiscard]] uint32_t AquireImage(bool *success);
        [[nodiscard]] bool PresentFinalImage(uint32_t imageIndex);

        void UpdateBuffers(const EngineData& engineData);

        ShiftWindow& m_window;
        std::shared_ptr<ctrl::FlyingCameraController> m_controller;

        ShiftBackBuffer m_backBuffer;
        ShiftContext m_context;

        std::unique_ptr<ModelSystem> m_modelSystem;
        std::unique_ptr<DescriptorManager> m_descriptorManager;
        std::unique_ptr<BufferManager> m_bufferManager;
        std::unique_ptr<SamplerManager> m_samplerManager;

        std::unique_ptr<TextureSystem> m_textureSystem;
        std::unique_ptr<GeometrySystem> m_meshSystem;
        std::unique_ptr<PostProcessSystem> m_postProcessSystem;
        std::unique_ptr<LightSystem> m_lightSystem;
        std::unique_ptr<RenderTargetSystem> m_RTSystem;

        uint32_t m_currentFrame = 0;

        SGUID m_perFrameID;
        std::unordered_map<ViewSetLayoutType, SGUID> m_perViewIDs;

        // Sync primitives to comtrol the rendering of a frame
        std::vector<std::unique_ptr<Semaphore>> m_imageAvailableSemaphores;
        std::vector<std::unique_ptr<Semaphore>> m_renderFinishedSemaphores;
    };
} // shift::gfx


#endif //SHIFT_RENDERER_HPP
