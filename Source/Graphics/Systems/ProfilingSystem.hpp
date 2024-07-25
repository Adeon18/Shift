//
// Created by otrush on 4/25/2024.
//

#ifndef SHIFT_PROFILINGSYSTEM_HPP
#define SHIFT_PROFILINGSYSTEM_HPP

#include <array>

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Commands/CommandBuffer.hpp"

#include "Graphics/UI/UIWindowComponent.hpp"

namespace shift::gfx {
    //! This system is aimed to fully release in Shift 1.1
    //! Currently serves a purpose of showing FPS and CPU and GPU timings
    class ProfilingSystem {
        class UI: public ui::UIWindowComponent {
        public:
            explicit UI(std::string name, std::string sName, ProfilingSystem& system): ui::UIWindowComponent{std::move(name), std::move(sName)}, m_system{system} {}

            virtual void Item() override { ui::UIWindowComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;
        private:
            ProfilingSystem& m_system;
        };

        /// We leave room for extension
        static constexpr uint32_t TIME_STAMPS_PER_FRAME = 10;
        static constexpr uint32_t PROFILER_BUFFER_SIZE = 2048;
    public:
        struct ProfilingStorage {
            std::array<float, PROFILER_BUFFER_SIZE> gpuTimeLog{};
            float gpuTimeAverage = 0.0f;
            float gpuTimeMax = 0.0f;
            float gpuTimeLast = 0.0f;

            std::array<float, PROFILER_BUFFER_SIZE> frameTimeLog{};
            float frameTimeAverage = 0.0f;
            float frameTimeMax = 0.0f;
            float frameTimeLast = 0.0f;
        };

        explicit ProfilingSystem(const Device& device);

        //! Calculate internal profiling data to display in the engine
        void UpdateProfileData(float currentFrameTime, uint32_t currentFrame);

        //! Reset the query pool of timings, must be done right after begin command buffer
        void ResetQueryPool(const CommandBuffer& buff);
        //! Put a timestamp at some CB
        void PutTimestamp(const CommandBuffer& buff, uint32_t currentFrame, VkPipelineStageFlagBits flags);

        void PollQueryPoolResults(uint32_t currentFrame);

        ~ProfilingSystem();

        ProfilingSystem(const ProfilingSystem&) = delete;
        ProfilingSystem& operator=(const ProfilingSystem&) = delete;
        ProfilingSystem() = delete;
    private:
        UI m_UI{"Profiling System", "Tools", *this};

        const Device& m_device;

        ProfilingStorage m_profilingStorage;

        VkQueryPool m_queryPool;
        std::array<uint64_t, 2u * TIME_STAMPS_PER_FRAME * shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT> m_timeStamps;
        uint32_t m_lastPutTimeStampIdx = 0u;

        uint64_t m_framesSinceStart = 0u;
    };
} // shift::gfx

#endif //SHIFT_PROFILINGSYSTEM_HPP
