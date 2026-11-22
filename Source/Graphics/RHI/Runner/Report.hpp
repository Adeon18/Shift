#ifndef SHIFT_RHI_REPORT_HPP
#define SHIFT_RHI_REPORT_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Shift::Samples {
    enum class EStatus { Passed, Failed, Updated, NotImplemented };

    struct SampleResult {
        std::string name;
        EStatus status = EStatus::Failed;
        std::string message;
        //! The stub that threw, empty unless NotImplemented
        std::string missing;
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

    //! Init and teardown, which belong to no sample
    struct StageResult {
        EStatus status = EStatus::Passed;
        std::string message;
        std::string missing;
    };

    struct RunReport {
        std::string backend;
        std::string device;
        uint32_t width = 0;
        uint32_t height = 0;
        double totalMs = 0.0;
        //! Unix epoch, so the page can show how old each backend's run is
        int64_t ranAtMs = 0;
        StageResult init;
        StageResult teardown;
        std::vector<SampleResult> samples;
    };

    //! results.js is loaded by a script tag, so report.html works from file://; each backend adds itself to window.SHIFT_RESULTS
    [[nodiscard]] bool WriteReport(const std::filesystem::path& folder, const RunReport& report);
} // Shift::Samples

#endif //SHIFT_RHI_REPORT_HPP
