#include "Report.hpp"

#include <format>
#include <fstream>

namespace Shift::Samples {
    namespace {
        std::string Quote(const std::string& text) {
            std::string out = "\"";
            for (const char c : text) {
                switch (c) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            out += std::format("\\u{:04x}", static_cast<int>(c));
                        } else {
                            out += c;
                        }
                }
            }
            return out + "\"";
        }

        const char* StatusName(EStatus status) {
            switch (status) {
                case EStatus::Passed: return "passed";
                case EStatus::Failed: return "failed";
                case EStatus::Updated: return "updated";
                case EStatus::NotImplemented: return "notimpl";
            }
            return "failed";
        }

        std::string Stage(const StageResult& stage) {
            return std::string("{\"status\": ") + Quote(StatusName(stage.status)) + ", \"message\": " + Quote(stage.message) +
                   ", \"missing\": " + Quote(stage.missing) + "}";
        }
    }

    bool WriteReport(const std::filesystem::path& folder, const RunReport& report) {
        std::ofstream file(folder / "results.js", std::ios::trunc);
        if (!file) {
            return false;
        }

        file << "(window.SHIFT_RESULTS = window.SHIFT_RESULTS || {})[" << Quote(report.backend) << "] = {\n";
        file << "  \"backend\": " << Quote(report.backend) << ",\n";
        file << "  \"device\": " << Quote(report.device) << ",\n";
        file << std::format("  \"width\": {},\n  \"height\": {},\n  \"totalMs\": {:.1f},\n  \"ranAtMs\": {},\n", report.width, report.height, report.totalMs, report.ranAtMs);
        file << "  \"init\": " << Stage(report.init) << ",\n";
        file << "  \"teardown\": " << Stage(report.teardown) << ",\n";
        file << "  \"samples\": [\n";
        for (size_t i = 0; i < report.samples.size(); ++i) {
            const SampleResult& s = report.samples[i];
            file << "    {";
            file << "\"name\": " << Quote(s.name);
            file << ", \"status\": " << Quote(StatusName(s.status));
            file << ", \"message\": " << Quote(s.message);
            file << ", \"missing\": " << Quote(s.missing);
            file << std::format(", \"runMs\": {:.2f}, \"flipMs\": {:.2f}, \"pngMs\": {:.2f}", s.runMs, s.flipMs, s.pngMs);
            file << std::format(", \"flipMean\": {:.6f}, \"threshold\": {:.4f}", s.flipMean, s.threshold);
            file << ", \"output\": " << Quote(s.output);
            file << ", \"golden\": " << Quote(s.golden);
            file << ", \"diff\": " << Quote(s.diff);
            file << (i + 1 < report.samples.size() ? "},\n" : "}\n");
        }
        file << "  ]\n};\n";
        return static_cast<bool>(file);
    }
} // Shift::Samples
