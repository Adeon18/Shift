//
// Created by otrush on 4/25/2024.
//

#include "ProfilingSystem.hpp"

namespace shift::gfx {
    ProfilingSystem::ProfilingSystem(const Device& device): m_device{device} {
        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = static_cast<uint32_t>(m_timeStamps.size());

        // TODO: Check for VK_NULL_HANDLE
        m_queryPool = m_device.CreateQueryPool(queryPoolInfo);
    }

    void ProfilingSystem::UpdateProfileData(float currentFrameTime) {
        m_framesSinceStart++;

        // Wait a second cuz why not
        if (m_framesSinceStart < 10000u) { return; }

        m_profilingStorage.gpuTimeLast = (m_timeStamps[0] > 0u && m_timeStamps[1] > 0u) ? static_cast<float>(m_timeStamps[1] - m_timeStamps[0]) * m_device.GetDeviceProperties().limits.timestampPeriod / 1'000'000.0f: m_profilingStorage.gpuTimeLast;
        m_profilingStorage.frameTimeLast = currentFrameTime;

        m_profilingStorage.gpuTimeTotal += m_profilingStorage.gpuTimeLast;
        m_profilingStorage.frameTimeTotal += m_profilingStorage.frameTimeLast;

        m_profilingStorage.gpuTimeMax = std::max(m_profilingStorage.gpuTimeMax, m_profilingStorage.gpuTimeLast);
        m_profilingStorage.frameTimeMax = std::max(m_profilingStorage.frameTimeMax, m_profilingStorage.frameTimeLast);

        m_profilingStorage.gpuTimeAverage = m_profilingStorage.gpuTimeTotal / static_cast<double>(m_framesSinceStart - 10000u);
        m_profilingStorage.frameTimeAverage = m_profilingStorage.frameTimeTotal / static_cast<double>(m_framesSinceStart - 10000u);
    }

    void ProfilingSystem::ResetQueryPool(const CommandBuffer &buff) {
        vkCmdResetQueryPool(buff.Get(), m_queryPool, 0, static_cast<uint32_t>(m_timeStamps.size()));
        m_lastPutTimeStampIdx = 0u;
    }

    void ProfilingSystem::PutTimestamp(const CommandBuffer &buff, VkPipelineStageFlagBits flags) {
        vkCmdWriteTimestamp(buff.Get(), flags, m_queryPool, m_lastPutTimeStampIdx++);
    }

    void ProfilingSystem::PollQueryPoolResults() {
        vkGetQueryPoolResults(
                m_device.Get(),
                m_queryPool,
                0,
                m_lastPutTimeStampIdx,
                m_timeStamps.size() * sizeof(uint64_t),
                m_timeStamps.data(),
                sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
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