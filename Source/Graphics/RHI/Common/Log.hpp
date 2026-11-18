#ifndef SHIFT_RHI_LOG_HPP
#define SHIFT_RHI_LOG_HPP

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace Shift::RHI {
    enum class ELogLevel : uint8_t { Trace, Debug, Info, Warning, Error, Critical };

    //! Set once before init as without one, messages go to stdout
    using LogCallback = void(*)(ELogLevel level, std::string_view message);
    void SetLogCallback(LogCallback callback);

    void WriteLogMessage(ELogLevel level, std::string_view message);

    template<typename... Args>
    void WriteLog(ELogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        WriteLogMessage(level, std::format(fmt, std::forward<Args>(args)...));
    }
} // Shift::RHI

#define SRHI_LOG(Level, ...) ::Shift::RHI::WriteLog(::Shift::RHI::ELogLevel::Level, __VA_ARGS__)

//! Return false (or nothing, for void functions) if the expression is false
#define SRHI_CHECK_CRITICAL(Expr, Msg) \
    do { \
        if (!(Expr)) { \
            SRHI_LOG(Critical, "Critical Check Failed: '{}' | {}:{} | {}", #Expr, __FILE__, __LINE__, Msg); \
            return false; \
        } \
    } while (0)

#define SRHI_CHECK_CRITICAL_EMPTY_RETURN(Expr, Msg) \
    do { \
        if (!(Expr)) { \
            SRHI_LOG(Critical, "Critical Check Failed: '{}' | {}:{} | {}", #Expr, __FILE__, __LINE__, Msg); \
            return; \
        } \
    } while (0)

#define SRHI_CHECK_EXIT(Expr) \
    do { \
        if (!(Expr)) { \
            return false; \
        } \
    } while (0)

#endif //SHIFT_RHI_LOG_HPP
