#include "UtilStandard.hpp"

#include <filesystem>

#include "Utility/Logging/LogMacros.hpp"

namespace Shift::Util {
    VersionTriple ParseVersionTriple(std::string_view version) {
        std::vector<std::string_view> tokens;
        StrSplitView(version, '.', &tokens);

        VersionTriple out{};
        uint32_t* dst[3] = { &out.uMajor, &out.uMinor, &out.uPatch };

        if (tokens.size() != 3) {
            Log(Warning, "Version '{}' has to have 3 version codes split by '.'. Defaulting to 1.0.0", version);
            return VersionTriple{};
        }

        for (size_t i = 0; i < 3; ++i) {
            const std::string_view token = tokens[i];
            auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), *dst[i]);
            if (ec != std::errc() || ptr != token.data() + token.size()) {
                Log(Warning, "Version '{}' has to have 3 NUMERIC version codes split by '.'. Defaulting to 1.0.0", version);
                return VersionTriple{};
            }
        }

        return out;
    }

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

    void StrSplitView(std::string_view str, char delimiter, std::vector<std::string_view> *outTokens) {
        size_t start = 0;
        while (true)
        {
            size_t pos = str.find(delimiter, start);
            if (pos == std::string_view::npos)
            {
                outTokens->emplace_back(str.substr(start));
                break;
            }
            outTokens->emplace_back(str.substr(start, pos - start));
            start = pos + 1;
        }
    }

    std::string GetDirectoryFromPath(const std::string& path)
    {
        return std::filesystem::path{ path }.parent_path().string() + "/";
    }

} // Shift::Util
