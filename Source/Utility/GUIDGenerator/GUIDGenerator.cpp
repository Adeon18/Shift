#include "GUIDGenerator.hpp"

namespace sft {
    namespace {
        std::random_device s_randDevice;
        std::mt19937_64 s_randEngine{s_randDevice()};
        std::uniform_int_distribution<uint64_t> s_distrib;
    }

    SGUID GUIDGenerator::Guid() const {
        return SGUID{s_distrib(s_randEngine)};
    }
}
