//
// Created by otrush on 4/25/2024.
//

#include <iostream>
#include <numeric>

#include "ProfilingSystem.hpp"

namespace Shift::gfx {
    ProfilingSystem::ProfilingSystem(const Device& device): m_device{device} {
        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = static_cast<uint32_t>(m_timeStamps.size());

        // TODO: Check for VK_NULL_HANDLE
        m_queryPool = m_device.CreateQueryPool(queryPoolInfo);
    }

    void ProfilingSystem::UpdateProfileData(float currentFrameTime, uint32_t currentFrame) {
        if (m_timeStamps[currentFrame * TIME_STAMPS_PER_FRAME * 2 + 1] == 0 ||
            m_timeStamps[currentFrame * TIME_STAMPS_PER_FRAME * 2 + 3] == 0)
        {
            return;
        }

        m_framesSinceStart++;

        m_profilingStorage.gpuTimeLast = static_cast<float>(m_timeStamps[currentFrame * TIME_STAMPS_PER_FRAME * 2 + 2] - m_timeStamps[currentFrame * TIME_STAMPS_PER_FRAME * 2]) * m_device.GetDeviceProperties().limits.timestampPeriod / 1'000'000.0f;
        m_profilingStorage.frameTimeLast = currentFrameTime;

        m_profilingStorage.gpuTimeLog[m_framesSinceStart % PROFILER_BUFFER_SIZE] = m_profilingStorage.gpuTimeLast;
        m_profilingStorage.frameTimeLog[m_framesSinceStart % PROFILER_BUFFER_SIZE] = m_profilingStorage.frameTimeLast;

        m_profilingStorage.gpuTimeMax = *std::max_element(m_profilingStorage.gpuTimeLog.begin(), m_profilingStorage.gpuTimeLog.end());
        m_profilingStorage.frameTimeMax = *std::max_element(m_profilingStorage.frameTimeLog.begin(), m_profilingStorage.frameTimeLog.end());

        m_profilingStorage.gpuTimeAverage = std::accumulate(m_profilingStorage.gpuTimeLog.begin(), m_profilingStorage.gpuTimeLog.end(), 0.f) / static_cast<float>(PROFILER_BUFFER_SIZE);
        m_profilingStorage.frameTimeAverage = std::accumulate(m_profilingStorage.frameTimeLog.begin(), m_profilingStorage.frameTimeLog.end(), 0.f) / static_cast<float>(PROFILER_BUFFER_SIZE);
    }

    void ProfilingSystem::ResetQueryPool(const CommandBuffer &buff) {
        vkCmdResetQueryPool(buff.Get(), m_queryPool, 0, static_cast<uint32_t>(m_timeStamps.size()));
        m_lastPutTimeStampIdx = 0u;
    }

    void ProfilingSystem::PutTimestamp(const CommandBuffer& buff, uint32_t currentFrame, VkPipelineStageFlagBits flags) {
        uint32_t timeStampIndex = currentFrame * TIME_STAMPS_PER_FRAME * 2u + m_lastPutTimeStampIdx;

        if (m_timeStamps[timeStampIndex] != 0) {
            vkCmdWriteTimestamp(buff.Get(), flags, m_queryPool, m_lastPutTimeStampIdx);
        }
        ++m_lastPutTimeStampIdx;
    }

    void ProfilingSystem::PollQueryPoolResults(uint32_t currentFrame) {
        vkGetQueryPoolResults(
                m_device.Get(),
                m_queryPool,
                0,
                m_lastPutTimeStampIdx,
                m_lastPutTimeStampIdx * 2 * sizeof(uint64_t),
                &m_timeStamps[TIME_STAMPS_PER_FRAME * Shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT * currentFrame],
                2 * sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
        );
    }

    ProfilingSystem::~ProfilingSystem() {
        m_device.DestroyQueryPool(m_queryPool);
    }

    void ProfilingSystem::UI::Show(uint32_t currentFrame) {
        if (m_shown) {
            ImGui::Begin(m_name.c_str(), &m_shown);

            std::string frameText = "Frame: " + std::to_string(m_system.m_framesSinceStart);
            ImGui::Text("%s", frameText.c_str());

            ImGui::Text("Timing table");
            if (ImGui::BeginTable("t1", 4))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4{0.0f, 1.0f, 0.0f, 1.0f}, "Average");

                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4{1.0f, 0.0f, 0.0f, 1.0f}, "Max");

                ImGui::TableSetColumnIndex(3);
                ImGui::TextColored(ImVec4{0.0f, 0.0f, 1.0f, 1.0f}, "Last");
                ///
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4{0.0f, 1.0f, 1.0f, 1.0f}, "GPU Time (ms)");

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%f", m_system.m_profilingStorage.gpuTimeAverage);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%f", m_system.m_profilingStorage.gpuTimeMax);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%f", m_system.m_profilingStorage.gpuTimeLast);
                ///
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4{0.0f, 1.0f, 1.0f, 1.0f}, "Frame Time (ms)");

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%f", m_system.m_profilingStorage.frameTimeAverage);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%f", m_system.m_profilingStorage.frameTimeMax);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%f", m_system.m_profilingStorage.frameTimeLast);

                ImGui::EndTable();
            }

            ImGui::End();
        }
    }
} // shift::gfx