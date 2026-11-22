#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "Graphics/RHI/Common/NotImplemented.hpp"
#include "Harness.hpp"
#include "Report.hpp"
#include "Sample.hpp"

using namespace Shift;
using namespace Shift::Samples;

namespace {
    struct Options {
        std::string filter;
        bool list = false;
        bool updateGoldens = false;
        uint32_t width = 1920;
        uint32_t height = 1080;
        std::filesystem::path out = "rhi_report";
    };

    bool ParseOptions(int argc, char** argv, Options& options) {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            const bool hasValue = i + 1 < argc;
            if (arg == "--list") {
                options.list = true;
            } else if (arg == "--update-goldens") {
                options.updateGoldens = true;
            } else if (arg == "--filter" && hasValue) {
                options.filter = argv[++i];
            } else if (arg == "--width" && hasValue) {
                options.width = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            } else if (arg == "--height" && hasValue) {
                options.height = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            } else if (arg == "--out" && hasValue) {
                options.out = argv[++i];
            } else {
                std::fprintf(stderr, "Unknown argument '%s'\n Usage: shift_rhi_samples [--list] [--filter <substr>] [--update-goldens] [--width N] [--height N] [--out <dir>]\n", arg.c_str());
                return false;
            }
        }
        return true;
    }

    std::string NotImplementedMessage(const RHINotImplemented& e) {
        return std::string("not implemented: ") + e.function;
    }

    //! The counters are process-global, so whatever a throw cut short is wiped by the next reset unless read here
    ValidationStats TryGetValidationStats() {
        try {
            return GetValidationStats();
        } catch (const RHINotImplemented&) {
            return {};
        }
    }

    double MsSince(std::chrono::steady_clock::time_point start) {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    }

    void AppendValidation(SampleResult& result, const ValidationStats& stats) {
        result.status = EStatus::Failed;
        std::string text = std::to_string(stats.errorCount) + " validation error(s), " + std::to_string(stats.warningCount) + " warning(s)";
        for (const std::string& message : stats.lastMessages) {
            text += "\n" + message;
        }
        result.message += result.message.empty() ? text : "\n" + text;
    }

    void CompareWithGolden(const Options& options, const Sample& sample, const SampleImage& image, bool validationClean, SampleResult& result) {
        const std::filesystem::path goldenPath = std::filesystem::path(SHIFT_RHI_GOLDENS_DIR) / (result.name + ".png");

        if (options.updateGoldens) {
            if (!validationClean) {
                result.message = "golden not written, the run had validation messages";
            } else if (WritePNG(goldenPath, image)) {
                result.status = EStatus::Updated;
                result.message = "golden written to " + goldenPath.string();
            } else {
                result.message = "failed to write " + goldenPath.string();
            }
            return;
        }

        SampleImage golden;
        if (!LoadPNG(goldenPath, golden)) {
            result.message = "no golden at " + goldenPath.string() + ", run with --update-goldens to create it";
            return;
        }
        std::error_code ec;
        if (std::filesystem::copy_file(goldenPath, options.out / (result.name + "_golden.png"), std::filesystem::copy_options::overwrite_existing, ec)) {
            result.golden = result.name + "_golden.png";
        }

        const auto flipStart = std::chrono::steady_clock::now();
        const ImageComparison comparison = CompareImages(golden, image);
        result.flipMs = MsSince(flipStart);

        if (!comparison.sizeMatches) {
            result.message = "golden is " + std::to_string(golden.width) + "x" + std::to_string(golden.height) + ", output is " + std::to_string(image.width) + "x" + std::to_string(image.height);
            return;
        }

        result.flipMean = comparison.flipMean;
        if (comparison.flipMean <= sample.flipThreshold) {
            result.status = EStatus::Passed;
            return;
        }

        result.message = "FLIP mean " + std::to_string(comparison.flipMean) + " > " + std::to_string(sample.flipThreshold);
        result.diff = result.name + "_diff.png";
        if (!WritePNG(options.out / result.diff, comparison.errorMap)) {
            result.diff.clear();
        }
    }

    SampleResult RunSample(const Options& options, RenderBackend& rhi, const Sample& sample) {
        SampleResult result{.name = sample.name, .threshold = sample.flipThreshold};
        SampleContext context{rhi, options.width, options.height};

        SampleOutput output;
        ValidationStats stats;
        try {
            ResetValidationStats();
            const auto runStart = std::chrono::steady_clock::now();
            output = sample.run(context);
            rhi.WaitForGPU();
            rhi.FlushAllDeferredCallbacks();
            result.runMs = MsSince(runStart);
            stats = GetValidationStats();
        } catch (const RHINotImplemented& e) {
            result.message = NotImplementedMessage(e);
            const ValidationStats raised = TryGetValidationStats();
            if (raised.errorCount + raised.warningCount > 0) {
                AppendValidation(result, raised);
            }
            return result;
        }
        const bool validationClean = stats.errorCount + stats.warningCount == 0;

        if (!output.error.empty()) {
            result.message = output.error;
        } else {
            result.output = result.name + ".png";
            const auto pngStart = std::chrono::steady_clock::now();
            if (!WritePNG(options.out / result.output, output.image)) {
                result.output.clear();
            }
            result.pngMs = MsSince(pngStart);
            CompareWithGolden(options, sample, output.image, validationClean, result);
        }

        if (!validationClean) {
            AppendValidation(result, stats);
        }
        return result;
    }
}

int main(int argc, char** argv) {
    Options options;
    if (!ParseOptions(argc, argv, options)) {
        return 1;
    }

    std::vector<const Sample*> selected;
    for (const Sample& sample : GetSamples()) {
        if (options.filter.empty() || std::strstr(sample.name, options.filter.c_str()) != nullptr) {
            selected.push_back(&sample);
        }
    }

    if (selected.empty()) {
        std::fprintf(stderr, "No sample matches '%s'\n", options.filter.c_str());
        return 1;
    }

    if (options.list) {
        for (const Sample* sample : selected) {
            std::printf("%s\n", sample->name);
        }
        return 0;
    }

    //! D3D12 copy footprints need 256-byte rows, pixel == 4b
    if (options.width == 0 || options.height == 0 || options.width % 64 != 0) {
        std::fprintf(stderr, "Width must be a non-zero multiple of 64 (got %ux%u)\n", options.width, options.height);
        return 1;
    }

    std::error_code ec;
    std::filesystem::create_directories(options.out, ec);
    if (ec) {
        std::fprintf(stderr, "Cannot create '%s': %s\n", options.out.string().c_str(), ec.message().c_str());
        return 1;
    }

    const auto totalStart = std::chrono::steady_clock::now();
    RunReport report{.backend = ShiftSelectedAPI::Name, .width = options.width, .height = options.height};

    RHIRequiredFeatures features = ShiftSelectedAPI::requiredFeatures;
    features.VK_syncValidation = true;
    const RHIAppInfo appInfo{.appName = "ShiftRHISamples", .engineName = "Shift"};

    RenderBackend rhi;
    bool initialized = false;
    std::string initError;
    ValidationStats init;
    try {
        ResetValidationStats();
        initialized = rhi.Init(NativeWindow{}, options.width, options.height, appInfo, {.maxImages = 16, .maxSamplers = 16}, features);
        init = GetValidationStats();
        if (initialized) {
            report.device = rhi.GetCapabilities().version.deviceName;
        }
    } catch (const RHINotImplemented& e) {
        initError = NotImplementedMessage(e);
        init = TryGetValidationStats();
    }

    if (initialized && initError.empty()) {
        for (const Sample* sample : selected) {
            report.samples.push_back(RunSample(options, rhi, *sample));
        }
    } else {
        for (const Sample* sample : selected) {
            report.samples.push_back({.name = sample->name, .message = initError.empty() ? "RHI init failed" : initError});
        }
    }

    std::string teardownError;
    ValidationStats teardown;
    try {
        ResetValidationStats();
        if (initialized) {
            rhi.Destroy();
        }
        teardown = GetValidationStats();
    } catch (const RHINotImplemented& e) {
        teardownError = NotImplementedMessage(e);
        teardown = TryGetValidationStats();
    }
    report.totalMs = MsSince(totalStart);

    if (!WriteReport(options.out, report)) {
        std::fprintf(stderr, "Failed to write results.js\n");
    }
    if (!std::filesystem::copy_file(SHIFT_RHI_REPORT_HTML, options.out / "report.html", std::filesystem::copy_options::overwrite_existing, ec)) {
        std::fprintf(stderr, "Failed to copy report.html: %s\n", ec.message().c_str());
    }

    int failed = 0;
    if (init.errorCount + init.warningCount > 0) {
        std::printf("FAIL     init: %llu validation error(s), %llu warning(s)\n",
                    static_cast<unsigned long long>(init.errorCount), static_cast<unsigned long long>(init.warningCount));
        ++failed;
    }
    for (const SampleResult& result : report.samples) {
        const char* status = result.status == EStatus::Passed ? "PASS" : result.status == EStatus::Updated ? "UPDATED" : "FAIL";
        std::printf("%-8s %-24s run %8.2f ms  png %7.2f ms  flip %8.2f ms  mean %.6f\n",
                    status, result.name.c_str(), result.runMs, result.pngMs, result.flipMs, result.flipMean);
        if (!result.message.empty()) {
            std::printf("         %s\n", result.message.c_str());
        }
        failed += result.status == EStatus::Failed ? 1 : 0;
    }
    if (!teardownError.empty()) {
        std::printf("FAIL     teardown: %s\n", teardownError.c_str());
        ++failed;
    }
    if (teardown.errorCount + teardown.warningCount > 0) {
        std::printf("FAIL     teardown: %llu validation error(s), %llu warning(s)\n",
                    static_cast<unsigned long long>(teardown.errorCount), static_cast<unsigned long long>(teardown.warningCount));
        ++failed;
    }
    std::printf("%zu sample(s), %d failed, %.1f ms total. Report: %s\n", report.samples.size(), failed, report.totalMs,
                std::filesystem::absolute(options.out / "report.html").string().c_str());
    return failed == 0 ? 0 : 1;
}
