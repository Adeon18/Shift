#include "Capabilities.hpp"
#include "Log.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>

namespace Shift {
    //! Teardown leak reports arrive while the RHI is mid-destruction, hence they static
    namespace {
        constexpr uint64_t MESSAGE_RING_DEPTH = 8;

        std::atomic<uint64_t> s_errors{0};
        std::atomic<uint64_t> s_warnings{0};
        std::atomic<uint64_t> s_infos{0};

        std::mutex s_ringMutex;
        std::array<std::string, MESSAGE_RING_DEPTH> s_ring;
        uint64_t s_ringNext = 0;

        void RecordMessage(const char* severity, std::string_view message) {
            std::lock_guard<std::mutex> guard(s_ringMutex);
            s_ring[s_ringNext % MESSAGE_RING_DEPTH] = std::string("[") + severity + "] " + std::string(message);
            ++s_ringNext;
        }
    }

    void ReportValidationMessage(EValidationSeverity severity, std::string_view message) {
        switch (severity) {
            case EValidationSeverity::Info:
                s_infos.fetch_add(1, std::memory_order_relaxed);
                SRHI_LOG(Info, "Validation layer: {}", message);
                break;
            case EValidationSeverity::Warning:
                s_warnings.fetch_add(1, std::memory_order_relaxed);
                RecordMessage("warning", message);
                SRHI_LOG(Warning, "Validation layer: {}", message);
                break;
            case EValidationSeverity::Error:
                s_errors.fetch_add(1, std::memory_order_relaxed);
                RecordMessage("error", message);
                SRHI_LOG(Error, "Validation layer: {}", message);
                break;
        }
    }

    ValidationStats GetValidationStats() {
        ValidationStats stats;
        stats.errorCount = s_errors.load(std::memory_order_relaxed);
        stats.warningCount = s_warnings.load(std::memory_order_relaxed);
        stats.infoCount = s_infos.load(std::memory_order_relaxed);

        std::lock_guard<std::mutex> guard(s_ringMutex);
        const uint64_t total = s_ringNext;
        const uint64_t depth = std::min<uint64_t>(total, MESSAGE_RING_DEPTH);
        stats.lastMessages.reserve(depth);
        for (uint64_t i = total - depth; i < total; ++i) {
            stats.lastMessages.push_back(s_ring[i % MESSAGE_RING_DEPTH]);
        }
        return stats;
    }

    void ResetValidationStats() {
        s_errors.store(0, std::memory_order_relaxed);
        s_warnings.store(0, std::memory_order_relaxed);
        s_infos.store(0, std::memory_order_relaxed);

        std::lock_guard<std::mutex> guard(s_ringMutex);
        for (auto& message : s_ring) {
            message.clear();
        }
        s_ringNext = 0;
    }
} // Shift
