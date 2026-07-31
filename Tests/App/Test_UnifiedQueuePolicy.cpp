//! Boots the engine with the queue roles collapsed onto the graphics family
//! (RHIRequiredFeatures::VK_forceUnifiedQueues) so the DEGENERATE branch of the queue-ownership
//! machinery executes on real hardware: when a texture's source and destination family are the
//! same, ReleaseQueueOwnership records a plain layout transition and nothing is ever handed off.
//!
//! Lives apart from Test_EngineSmoke.cpp on purpose: that file is deliberately backend-agnostic
//!  and this one has to look at Vulkan queue families
//! to prove the policy actually collapsed.
//! Suite tag: [app] (filter with: shift_tests -ts=*[app]*). Needs a GPU + driver.
#include <doctest/doctest.h>

#include <string>

#include "ShiftEngine.hpp"
#include "Graphics/RHI/Common/Capabilities.hpp"

//! Whole file, not just the family check below: queue FAMILIES are a Vulkan concept (DX12 has
//! fixed queue types and no ownership transfer at all). Without this guard the case still
//! compiles on another backend — VK_forceUnifiedQueues lives in the agnostic feature struct —
//! and passes vacuously, asserting nothing. Test_EngineSmoke.cpp stays unguarded on purpose:
//! it is backend-agnostic and is the P7 boot conformance case
#ifdef SHIFT_VULKAN_BACKEND

namespace {
    //! Determinism policy: tests never depend on FPSTimer's real-time pacing
    constexpr float FIXED_DT = 1.0f / 60.0f;
}

TEST_SUITE("EngineUnifiedQueues [app]") {

TEST_CASE("the unified queue policy boots, renders and tears down validation-clean") {
    //! Isolate the process-global validation counters from the other [app] boot
    Shift::ResetValidationStats();

    Shift::RHIRequiredFeatures features = Shift::ShiftSelectedAPI::requiredFeatures;
    features.VK_syncValidation = true;
    features.VK_forceUnifiedQueues = true;

    Shift::EngineDescriptor desc;
    desc.width = 640;
    desc.height = 480;
    desc.windowName = "ShiftUnifiedQueueTest";
    desc.windowVisible = false;
    desc.featuresOverride = features;

    Shift::ShiftEngine engine;
    REQUIRE_MESSAGE(engine.Init(desc), "engine failed to initialize ([app] tests need a GPU/driver)");

    auto& renderer = engine.GetRenderer();

    //! A green run proves nothing on its own here. Had the policy silently not collapsed, the
    //! ordinary split path would have run and passed just as quietly. This is the check that the
    //! branch under test is the one that executed: with a single resource family, src == dst
    //! inside ReleaseQueueOwnership by construction, for every texture, on every upload
    const auto& local = renderer.GetRHILocal<Shift::ShiftSelectedAPI>();
    const size_t resourceFamilyCount = local.device->GetUniqueQueueFamilyIndices().size();
    REQUIRE_MESSAGE(resourceFamilyCount == 1,
                    "VK_forceUnifiedQueues did not collapse the resource queue families, so the "
                    "same-family branch went untested");

    //! No mid-run upload here on purpose: Renderer::Init already uploads the placeholder and
    //! NB.jpg through ReleaseQueueOwnership on the transfer context, which under this policy
    //! shares the graphics family — so the same-family branch has already run twice before the
    //! first frame. Repeating it mid-run would only duplicate what the split-family smoke covers.
    //! These frames are here to render, present and cycle the frames-in-flight slots
    for (int i = 0; i < 10; ++i) {
        REQUIRE(engine.Tick(FIXED_DT));
    }

    engine.Cleanup();

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

#endif //SHIFT_VULKAN_BACKEND
