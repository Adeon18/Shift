//
// Created by otrush on 10/30/2025.
//

#ifndef SHIFT_ASSERTIONS_HP
#define SHIFT_ASSERTIONS_HP

#include "Utility/Logging/LogMacros.hpp"

//! Check and return false if the statement is false
#define CheckCritical(expr, msg) \
    do { \
        if (!(expr)) { \
            LogCritical("Critical Check Failed: '{}' | {}:{} | {}", #expr, __FILE__, __LINE__, msg); \
            return false; \
        } \
    } while (0)


#define CheckExit(expr) \
    do { \
        if (!(expr)) { \
            return false; \
        } \
    } while (0)

#endif //SHIFT_ASSERTIONS_HP