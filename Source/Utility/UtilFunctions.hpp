#pragma once

#include <string>

namespace sft {
    namespace utl {
        constexpr std::string GetShiftRoot() {
            return std::string{SHIFT_ROOT} + "/";
        }
    }
}