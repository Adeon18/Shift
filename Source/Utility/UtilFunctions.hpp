#ifndef SHIFT_UTILITY_HPP
#define SHIFT_UTILITY_HPP

#include <string>

namespace sft {
    namespace utl {
        constexpr std::string GetShiftRoot() {
            return std::string{SHIFT_ROOT} + "/";
        }
    }
}

#endif //SHIFT_UTILITY_HPP