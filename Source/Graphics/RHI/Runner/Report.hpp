#ifndef SHIFT_RHI_REPORT_HPP
#define SHIFT_RHI_REPORT_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Shift::Samples {
    enum class EStatus { Passed, Failed, Updated };

    struct SampleResult {
        std::string name;
        EStatus status = EStatus::Failed;
        std::string message;
        double runMs = 0.0;
        double flipMs = 0.0;
        double pngMs = 0.0;
        float flipMean = -1.0f;
        float threshold = 0.0f;
        //! Relative to the report folder, empty when absent
        std::string output;
        std::string golden;
        std::string diff;
    };

    struct RunReport {
        std::string backend;
        std::string device;
        uint32_t width = 0;
        uint32_t height = 0;
        double totalMs = 0.0;
        std::vector<SampleResult> samples;
    };

    //! results.js is loaded by a script tag, so report.html works from file://
    [[nodiscard]] bool WriteReport(const std::filesystem::path& folder, const RunReport& report);
} // Shift::Samples

#endif //SHIFT_RHI_REPORT_HPP
