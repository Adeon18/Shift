#ifndef SHIFT_UTILITY_HPP
#define SHIFT_UTILITY_HPP

#include <string>
#include <vector>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>

namespace shift {
    namespace util {
        constexpr std::string GetShiftRoot() {
            return std::string{SHIFT_ROOT} + "/";
        }

        //! TODO: Can be optimized!
        [[nodiscard]] std::vector<char> ReadFile(const std::string& filename);

        [[nodiscard]] std::string GetDirectoryFromPath(const std::string& path);

        namespace ass {
            glm::vec3 ToGlm(const aiVector3D& vec);

            glm::mat4 ToGlm(const aiMatrix4x4& mat);
        }
    }
}

#endif //SHIFT_UTILITY_HPP