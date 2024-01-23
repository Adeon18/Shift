#include <string>

namespace zp {
    namespace utl {
        constexpr std::string GetZapRoot() {
            return std::string{ZAP_ROOT} + "/";
        }
    }
}