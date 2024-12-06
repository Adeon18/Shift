//
// Created by otrush on 11/5/2024.
//

#ifndef SHIFT_LOGMACROS_HPP
#define SHIFT_LOGMACROS_HPP

#include "spdlog/spdlog.h"

#define Log(Level, ...) Log##Level(__VA_ARGS__)
#define LogVerbose(Level, ...) LogVerbose##Level(__VA_ARGS__)

#define LogTrace(...) spdlog::trace(__VA_ARGS__)
#define LogDebug(...) spdlog::debug(__VA_ARGS__)
#define LogInfo(...) spdlog::info(__VA_ARGS__)
#define LogWarning(...) spdlog::warn(__VA_ARGS__)
#define LogWarn(...) spdlog::warn(__VA_ARGS__)
#define LogError(...) spdlog::error(__VA_ARGS__)
#define LogCritical(...) spdlog::critical(__VA_ARGS__)

#define LogVerboseTrace(...) SPDLOG_TRACE(__VA_ARGS__)
#define LogVerboseDebug(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LogVerboseInfo(...) SPDLOG_INFO(__VA_ARGS__)
#define LogVerboseWarning(...) SPDLOG_WARN(__VA_ARGS__)
#define LogVerboseWarn(...) SPDLOG_WARN(__VA_ARGS__)
#define LogVerboseError(...) SPDLOG_ERROR(__VA_ARGS__)
#define LogVerboseCritical(...) SPDLOG_CRITICAL(__VA_ARGS__)



#endif //SHIFT_LOGMACROS_HPP
