//! Boots the REAL engine (window hidden, sync-validation forced on) and
//! tests a full engine run.
//! Suite tag: [app] (filter with: shift_tests -ts=*[app]*). Needs a GPU + driver.
#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "ShiftEngine.hpp"
#include "Graphics/Managers/TextureManager.hpp"
#include "Graphics/RHI/Common/Capabilities.hpp"
#include "Utility/UtilStandard.hpp"

#include <GLFW/glfw3.h>

namespace {
    //! Determinism policy: tests never depend on FPSTimer's real-time pacing
    constexpr float FIXED_DT = 1.0f / 60.0f;
}

TEST_SUITE("EngineSmoke [app]") {

//! One linear case on purpose: engine boot is expensive and the phases build on each other
//! (doctest SUBCASEs would re-boot the engine per subcase)
TEST_CASE("the engine survives a scripted frame storm validation-clean") {
    //! Isolate the process-global validation counters from anything that ran before us
    Shift::ResetValidationStats();

    //! The backend's standard feature set, plus sync-validation
    Shift::RHIRequiredFeatures features = Shift::ShiftSelectedAPI::requiredFeatures;
    features.VK_syncValidation = true;

    Shift::EngineDescriptor desc;
    desc.width = 1080;
    desc.height = 720;
    desc.windowName = "ShiftSmokeTest";
    desc.windowVisible = false;
    desc.featuresOverride = features;

    Shift::ShiftEngine engine;
    REQUIRE_MESSAGE(engine.Init(desc), "engine failed to initialize ([app] tests need a GPU/driver)");

    //! Ssample the resolved GPU zones every frame and keep the max 'Frame' duration.
    //! A hidden-window frame may record no passes (Frame span ~0), so we can't trust whichever
    //! frame is collected last -- but frame 0 forces a viewport pass, so a working timestamp
    //! pipeline must yield at least one positive sample somewhere across the run.
    float maxFrameMs = 0.0f;
    bool sawFrameZone = false;
    Shift::DebugLabelColor frameColor{};

    auto tick = [&](int frames) {
        for (int i = 0; i < frames; ++i) {
            REQUIRE(engine.Tick(FIXED_DT));
            for (const Shift::GPUTimeRange& range : engine.GetRenderer().GetLastFrameGPUTimeRanges()) {
                if (range.name == "Frame") {
                    sawFrameZone = true;
                    maxFrameMs = std::max(maxFrameMs, range.milliseconds);
                    frameColor = range.color;
                }
            }
        }
    };

    GLFWwindow* windowHandle = engine.GetWindow().GetHandle();

    //! Phase 1: steady warmup
    tick(30);

    //! Phase 2: OS-window resize
    //! Exercises OUT_OF_DATE/SUBOPTIMAL acquires, swapchain + per-image semaphore recreation,
    //! and the abandoned-frame path (acquire fails, frame skipped, no submit)
    const std::pair<int, int> windowSizes[] = {
        {640, 480}, {333, 777}, {1280, 800}, {200, 150}, {1080, 720},
    };
    for (const auto& [width, height] : windowSizes) {
        glfwSetWindowSize(windowHandle, width, height);
        tick(3);
    }
    //! Burst: several resizes coalesced into one frame's event poll
    glfwSetWindowSize(windowHandle, 500, 500);
    glfwSetWindowSize(windowHandle, 600, 400);
    glfwSetWindowSize(windowHandle, 1080, 720);
    tick(3);

    //! Phase 3: iconify/restore (0-size frames: present is skipped, viewport still renders)
    glfwIconifyWindow(windowHandle);
    tick(1);
    //! Minimized client size is OS policy, not ours: report (without failing) if the
    //! skip-present path did not actually engage on this platform
    const bool iconifiedToZeroSize =
            engine.GetWindow().GetWidth() == 0 || engine.GetWindow().GetHeight() == 0;
    WARN_MESSAGE(iconifiedToZeroSize,
                 "iconify did not yield a 0-sized framebuffer; the skip-present path went unexercised");
    tick(4);
    glfwRestoreWindow(windowHandle);
    tick(1);
    //! Win32 restore can force a hidden window visible; re-hide to keep the run windowless
    glfwHideWindow(windowHandle);
    tick(4);

    //! Phase 4: viewport render-target resize storm
    //! Every resize recreates the RT, re-registers the ImGui descriptor and defers the old
    //! texture's delete to the next frame: 30 in a row is a deferred-destruction soak
    auto& renderer = engine.GetRenderer();
    for (int i = 0; i < 30; ++i) {
        const uint32_t width = 160 + static_cast<uint32_t>((i * 97) % 800);
        const uint32_t height = 120 + static_cast<uint32_t>((i * 61) % 600);
        renderer.ResizeViewport(width, height);
        tick(1);
    }
    renderer.ResizeViewport(0, 0);      //! Guard path: must be a harmless no-op
    renderer.ResizeViewport(1, 1);      //! Smallest valid render target
    tick(2);
    renderer.ResizeViewport(1, 1);      //! Same-size early-out path
    tick(2);

    //! Phase 5: shader hot-reload (watcher deliberately bypassed: MarkDirty is used)
    //! Real recompile of the live vertex shader, in-place pipeline rebuild, retired GPU handle
    //! released via the deferred executor once the timeline passes
    const std::string triangleVS = Shift::Util::GetShiftShaderSrcDir() + "Debug/TriangleVS.slang";
    renderer.GetShaderManager().MarkDirty(triangleVS);
    const uint32_t rebuiltCount = renderer.HotReloadShaders();
    CHECK_MESSAGE(rebuiltCount == 1,
                  "expected exactly the triangle pipeline to rebuild after marking its VS dirty");
    tick(10);

    //! Mid-run texture upload
    //! The point is that this runs on a warm engine rather than during boot: the copy goes out on
    //! the transfer queue between two frames, crosses to graphics as a queue-ownership
    //! release/acquire pair, and has its bindless slot written in the very frame that acquires it
    auto& textures = renderer.GetTextureManager();
    const Shift::Graphics::TextureHandle uploaded =
            textures.LoadTextureDeferred(Shift::Util::GetShiftRoot() + "Assets/Textures/ShiftIcon.jpg");
    CHECK_MESSAGE(textures.IsValid(uploaded), "mid-run texture load did not produce a live slot");
    //! Submit happens on the next frame, the acquire and the bindless write on that same frame
    tick(5);
    CHECK_MESSAGE(textures.IsValid(uploaded), "the mid-run texture did not survive its own upload");

    //! A file that does not exist must resolve to the placeholder: a usable, always-resident slot.
    //! Not a live slot holding nullptr (the next ForEachLive sweep would dereference it) and not
    //! an invalid handle either, since slotIdx is the bindless index a shader would go on to use
    const Shift::Graphics::TextureHandle missing =
            textures.LoadTextureDeferred(Shift::Util::GetShiftRoot() + "Assets/Textures/NoSuchTexture.png");
    CHECK_MESSAGE(textures.IsValid(missing), "a failed load handed out an unusable handle");
    CHECK_MESSAGE(missing == textures.GetPlaceholderHandle(),
                  "a failed load did not fall back to the placeholder");

    //! Unloading the fallback must be refused: it is shared by every slot that has no image yet,
    //! so releasing it would strand those descriptors on a deleted image
    textures.UnloadTexture(missing);
    CHECK_MESSAGE(textures.IsValid(textures.GetPlaceholderHandle()),
                  "the placeholder was unloaded out from under every slot referencing it");

    //! ...and none of that wedged the pending-upload list
    tick(3);

    //! Phase 7: late deferred callbacks land while frames keep flowing
    tick(60);

    //! Over the whole run at least one measured frame did real GPU work (frame 0
    //! forces a viewport pass), so a working timestamp pipeline must have produced a positive
    //! 'Frame' duration. Asserting on the running max (not the last frame) tolerates hidden-window
    //! frames that legitimately record no passes and read ~0.
    CHECK_MESSAGE(sawFrameZone, "no top-level 'Frame' GPU zone ever resolved");
    CHECK_MESSAGE(maxFrameMs > 0.0f,
                  "GPU 'Frame' zone never had a positive duration -- timestamps are not resolving");
    CHECK(maxFrameMs < 1000.0f);

    //! Check if frame debug range color has a timing color as well
    const bool frameColored = (frameColor.r > 0.0f) || (frameColor.g > 0.0f) || (frameColor.b > 0.0f) || (frameColor.a > 0.0f);
    CHECK_MESSAGE(frameColored, "timed debug-group color did not thread through to the resolved GPU range");

    engine.Cleanup();

    //! The whole run, including teardown leak checks, was clean
    //! The sink is static storage, so teardown reports arriving mid-RHI-destruction are counted
    const Shift::ValidationStats stats = Shift::GetValidationStats();
    std::string ring;
    for (const std::string& message : stats.lastMessages) {
        ring += message;
        ring += '\n';
    }
    CHECK_MESSAGE(stats.errorCount == 0, "validation reported errors; most recent messages:\n", ring);
    if (stats.warningCount > 0) {
        //! Errors-only bar for now (TESTING_PLAN open decision); warnings stay visible
        MESSAGE("validation warnings (not failing the gate yet): ", stats.warningCount, "\n", ring);
    }
}

}
