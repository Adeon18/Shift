#ifndef SHIFT_UTILITY_HPP
#define SHIFT_UTILITY_HPP

#include <string>
#include <vector>
#include <fstream>
#include <charconv>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>

namespace Shift::Util {
    constexpr std::string GetShiftRoot() {
        return std::string{SHIFT_ROOT} + "/";
    }

    constexpr std::string GetShiftShaderBuildDir() {
        return std::string{SHIFT_ROOT} + "/Shaders/Build/";
    }

    constexpr std::string StrToLower(const std::string& str) {
        std::string transformed;
        transformed.resize(str.size());
                std::transform(str.begin(), str.end(), transformed.begin(),
                              [](unsigned char c){ return std::tolower(c); });
        return transformed;
    }

    void StrSplitView(std::string_view str, char delimiter, std::vector<std::string_view> *outTokens);

    inline bool StrToUint32Fast(std::string_view str, uint32_t& outValue)
    {
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), outValue);
        return ec == std::errc();
    }


    //! TODO: Can be optimized!
    [[nodiscard]] std::vector<char> ReadFile(const std::string& filename);

    [[nodiscard]] std::string GetDirectoryFromPath(const std::string& path);

    namespace Ass {
        glm::vec3 ToGlm(const aiVector3D& vec);

        glm::mat4 ToGlm(const aiMatrix4x4& mat);
    }
} // Shift::Util

#endif //SHIFT_UTILITY_HPP