#include "UtilStandard.hpp"

namespace sft {
    namespace util {
        std::vector<char> ReadFile(const std::string& filename) {
            // We start reading at the end in order to get the file size
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            std::vector<char> buffer;
            if (!file.is_open()) {
                return buffer;
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            buffer.resize(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();
            return buffer;
        }
    } // util
} // sft
