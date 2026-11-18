#include "Graphics/RHI/Common/Log.hpp"

#include <cstdio>

namespace Shift::RHI {
    namespace {
        LogCallback g_logCallback = nullptr;

        const char* LevelName(ELogLevel level) {
            switch (level) {
                case ELogLevel::Trace: return "trace";
                case ELogLevel::Debug: return "debug";
                case ELogLevel::Info: return "info";
                case ELogLevel::Warning: return "warning";
                case ELogLevel::Error: return "error";
                case ELogLevel::Critical: return "critical";
            }
            return "unknown";
        }
    }

    void SetLogCallback(LogCallback callback) {
        g_logCallback = callback;
    }

    void WriteLogMessage(ELogLevel level, std::string_view message) {
        if (g_logCallback) {
            g_logCallback(level, message);
            return;
        }
        std::printf("[%s] %.*s\n", LevelName(level), static_cast<int>(message.size()), message.data());
    }
} // Shift::RHI
