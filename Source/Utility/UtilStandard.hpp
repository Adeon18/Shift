#ifndef SHIFT_UTILITY_HPP
#define SHIFT_UTILITY_HPP

#include <string>
#include <vector>
#include <fstream>

namespace sft {
    namespace util {
        constexpr std::string GetShiftRoot() {
            return std::string{SHIFT_ROOT} + "/";
        }

        //! TODO: Can be optimized!
        [[nodiscard]] std::vector<char> ReadFile(const std::string& filename);
    }
}

#endif //SHIFT_UTILITY_HPP