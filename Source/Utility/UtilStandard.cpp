#include "UtilStandard.hpp"

#include <filesystem>

namespace shift {
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

        std::string GetDirectoryFromPath(const std::string& path)
        {
            return std::filesystem::path{ path }.parent_path().string() + "/";
        }

        namespace ass {
            glm::vec3 ToGlm(const aiVector3D& vec)
            {
                return { vec.x, vec.y, vec.z };
            }

            glm::mat4 ToGlm(const aiMatrix4x4& mat)
            {
                return glm::transpose(glm::make_mat4(&mat.a1));
            }
        }
    } // util
} // shift
