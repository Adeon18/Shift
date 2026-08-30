//! Boots the REAL engine (window hidden, sync-validation forced on) and
//! tests a full engine run.
//! Suite tag: [app] (filter with: shift_tests -ts=*[app]*). Needs a GPU + driver.
#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "ShiftEngine.hpp"
#include "Config/EngineConfig.hpp"
#include "Graphics/DrawItem.hpp"
#include "Graphics/Managers/MeshManager.hpp"
#include "Graphics/RenderScene.hpp"
#include "Graphics/Managers/TextureManager.hpp"
#include "Graphics/RHI/Common/Capabilities.hpp"
#include "Graphics/Shared/GPUShared.h"
#include "Input/Keyboard.hpp"
#include "Utility/UtilStandard.hpp"

#include <GLFW/glfw3.h>

namespace {
    //! Determinism policy: tests never depend on FPSTimer's real-time pacing
    constexpr float FIXED_DT = 1.0f / 60.0f;

    //! Largest absolute element difference between two matrices. Matrix equality needs a
    //! tolerance: viewProj is composed once on the CPU and recomposed here from its own factors
    float MaxAbsDiff(const glm::mat4& lhs, const glm::mat4& rhs) {
        float worst = 0.0f;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                worst = std::max(worst, std::fabs(lhs[col][row] - rhs[col][row]));
            }
        }
        return worst;
    }

    float MaxAbs(const glm::mat4& matrix) {
        return MaxAbsDiff(matrix, glm::mat4(0.0f));
    }

    //! Everything RenderScene::Extract produced, checked against the meshes it came from.
    //! A free function rather than a block inside the case so an early bail is safe: a `return`
    //! from the case itself would skip engine.Cleanup() and tear the RHI down with work still in
    //! flight, burying one honest failure under a wall of validation errors and a VMA abort
    void CheckExtractedFrame(Shift::Graphics::Renderer& renderer) {
        auto& meshes = renderer.GetMeshManager();
        const auto& placements = renderer.GetPlacements();
        const auto& scene = renderer.GetRenderScene();
        const auto& objects = scene.GetObjects();
        const auto& items = scene.GetDrawItems();
        const Shift::Graphics::RenderSceneStats& stats = scene.GetStats();

        REQUIRE_MESSAGE(!placements.empty(), "the boot scene uploaded no meshes at all");
        CHECK_MESSAGE(meshes.GetUsedVertices() > 0, "the merged vertex streams hold nothing");
        CHECK_MESSAGE(meshes.GetUsedIndices() > 0, "the merged index buffer holds nothing");

        //! Everything below reads what Extract produced, so an unwritten Extract fails here rather
        //! than in twenty confusing places downstream
        const bool extractionRan = !objects.empty() && !items.empty();
        CHECK_MESSAGE(extractionRan,
                      "extraction produced nothing: RenderScene::Extract is not filling the object "
                      "array or the draw list");
        if (!extractionRan) {
            MESSAGE("skipping the remaining extraction checks; teardown still runs cleanly");
            return;
        }

        //! Every check below indexes one array with the other's index, so a mismatch here does not
        //! just fail once - it turns every later comparison into a draw item checked against an
        //! unrelated mesh. Bail on the one real failure instead of reporting a dozen fake ones
        const bool oneObjectPerPlacement = objects.size() == placements.size();
        CHECK_MESSAGE(oneObjectPerPlacement,
                      "one ObjectData per placement is the whole contract; a mismatch means "
                      "extraction skipped or duplicated one");
        if (!oneObjectPerPlacement) { return; }
        CHECK(stats.objects == static_cast<uint32_t>(objects.size()));
        CHECK(stats.drawCalls == static_cast<uint32_t>(items.size()));
        CHECK(stats.placements == static_cast<uint32_t>(placements.size()));

        const Shift::GPU::ObjectData* ringObjects = renderer.GetObjectData(0);
        REQUIRE_MESSAGE(ringObjects != nullptr, "object ring slot 0 does not resolve");

        for (size_t i = 0; i < objects.size(); ++i) {
            CAPTURE(i);
            const Shift::Graphics::Mesh* mesh = meshes.Get(placements[i].mesh);
            REQUIRE_MESSAGE(mesh != nullptr, "a placement points at no mesh");

            CHECK_MESSAGE(mesh->vertexRange.count > 0, "a mesh was uploaded with no vertices");
            CHECK_MESSAGE(mesh->indexRange.count > 0, "a mesh was uploaded with no indices");
            CHECK_MESSAGE(!mesh->submeshes.empty(), "a mesh was uploaded with no submeshes");

            //! The composed node transform times the framing transform, so a zeroed slot shows up
            CHECK(MaxAbs(objects[i].model) > 0.0f);
            CHECK(objects[i].boundsSphere.w > 0.0f);

            //! boundsSphere is WORLD space, so its radius must carry the placement's scale.
            //! Checking only that it is positive passes for a radius that was never scaled at all
            //! - and an over-large radius stays invisible until P4.4's cull under-rejects.
            //! Largest basis vector, not the first: a composed node transform may be non-uniform
            const glm::mat4& xform = placements[i].transform;
            const float placementScale = std::max({glm::length(glm::vec3(xform[0])),
                                                   glm::length(glm::vec3(xform[1])),
                                                   glm::length(glm::vec3(xform[2]))});
            CHECK_MESSAGE(std::fabs(objects[i].boundsSphere.w - mesh->bounds.sphere.w * placementScale) < 1e-3f,
                          "the world-space bounds radius does not equal the mesh radius times the "
                          "placement's scale");

            //! The extracted array is what the ring receives, byte for byte
            CHECK(MaxAbsDiff(ringObjects[i].model, objects[i].model) == 0.0f);
        }

        uint32_t instanceTotal = 0;
        for (const Shift::Graphics::DrawItem& item : items) {
            REQUIRE_MESSAGE(item.objectIndex < objects.size(),
                            "a draw item addresses an object that does not exist");
            const Shift::Graphics::Mesh* mesh = meshes.Get(placements[item.objectIndex].mesh);
            REQUIRE(mesh != nullptr);

            //! Every draw passes vertexOffset = 0, so THIS is the only thing that puts a mesh's
            //! vertices at the right place in the merged streams. A drift renders another mesh
            CHECK_MESSAGE(item.meshVertexBase == mesh->vertexRange.first,
                          "PushConstants::meshVertexBase does not match where the mesh landed");

            //! firstIndex is absolute in the merged buffer: the mesh's base plus the submesh's own
            //! offset. Missing the base and the draw reads a neighbouring mesh's triangles
            CHECK(item.firstIndex >= mesh->indexRange.first);
            const bool insideMesh =
                    item.firstIndex + item.indexCount <= mesh->indexRange.first + mesh->indexRange.count;
            CHECK_MESSAGE(insideMesh, "a draw item's index range runs past its own mesh");
            CHECK(item.indexCount > 0u);
            CHECK(item.instanceCount == 1u);
            CHECK_MESSAGE(Shift::Graphics::HasPass(item.passMask, Shift::Graphics::EPassBit::Forward),
                          "a draw item is in no pass, so nothing would ever record it");

            instanceTotal += item.instanceCount;
        }
        CHECK(stats.instances == instanceTotal);

        //! Sorted by sortKey, which is what makes P4.4's collapse a single linear scan
        const bool sorted = std::is_sorted(items.begin(), items.end(),
            [](const Shift::Graphics::DrawItem& a, const Shift::Graphics::DrawItem& b) {
                return a.sortKey < b.sortKey;
            });
        CHECK_MESSAGE(sorted, "the draw list is not sorted by sortKey");

        //! Every vertex the allocator handed out belongs to exactly one mesh. Summed over UNIQUE
        //! meshes, not placements: several placements may share one mesh, which is the whole point
        //! of uploading meshes once and placing them per node
        std::vector<uint32_t> uniqueMeshSlots;
        uint32_t vertexTotal = 0;
        for (const auto& placement : placements) {
            const uint32_t slot = placement.mesh.slotIdx;
            if (std::find(uniqueMeshSlots.begin(), uniqueMeshSlots.end(), slot) != uniqueMeshSlots.end()) {
                continue;
            }
            uniqueMeshSlots.push_back(slot);
            const Shift::Graphics::Mesh* mesh = meshes.Get(placement.mesh);
            REQUIRE(mesh != nullptr);
            vertexTotal += mesh->vertexRange.count;
        }
        CHECK_MESSAGE(vertexTotal == meshes.GetUsedVertices(),
                      "the allocator has handed out a different number of vertices than the meshes "
                      "actually hold, so a range leaked or was double-counted");

        //! With more than one mesh they cannot all start at vertex 0, which is what makes the mesh
        //! base load-bearing rather than trivially correct. The committed skull is four meshes, so
        //! this holds without the downloaded helmet
        if (placements.size() > 1) {
            const Shift::Graphics::Mesh* first = meshes.Get(placements[0].mesh);
            const Shift::Graphics::Mesh* second = meshes.Get(placements[1].mesh);
            REQUIRE(first != nullptr);
            REQUIRE(second != nullptr);
            const bool disjoint =
                    second->vertexRange.first >= first->vertexRange.first + first->vertexRange.count;
            CHECK_MESSAGE(second->vertexRange.first != 0u,
                          "the second mesh also starts at vertex 0: the suballocator handed out the "
                          "same range twice");
            CHECK_MESSAGE(disjoint, "two meshes overlap in the merged vertex streams");
        } else {
            MESSAGE("the boot scene holds a single mesh, so a non-zero mesh base vertex went "
                    "untested this run");
        }
    }
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
    //! How many frames actually recorded a scene pass. The depth attachment and the draws only
    //! exist inside it, so this is what says whether the run exercised them at all - and whether it
    //! exercised them on OVERLAPPING frames, which is the only way a cross-frame depth hazard could
    //! show up in sync validation
    uint32_t viewportPassFrames = 0;

    auto tick = [&](int frames) {
        for (int i = 0; i < frames; ++i) {
            REQUIRE(engine.Tick(FIXED_DT));
            for (const Shift::GPUTimeRange& range : engine.GetRenderer().GetLastFrameGPUTimeRanges()) {
                if (range.name == "Frame") {
                    sawFrameZone = true;
                    maxFrameMs = std::max(maxFrameMs, range.milliseconds);
                    frameColor = range.color;
                }
                if (range.name == "ViewportPass") {
                    ++viewportPassFrames;
                }
            }
        }
    };

    GLFWwindow* windowHandle = engine.GetWindow().GetHandle();

    //! Phase 1: steady warmup
    tick(30);

    //! Phase 1.5: the global sampler array is a cache, not a fixed table.
    //! Boot already registered the states shader code names by constant; what matters here is that
    //! the array behaves like a cache on both sides - identical states share a slot, and a state
    //! nobody thought of at startup can still be had
    {
        auto& samplers = engine.GetRenderer().GetSamplerManager();

        const uint32_t namedCount = samplers.GetCount();
        CHECK_MESSAGE(namedCount == 4u,
                      "boot did not end with exactly the four named samplers; either one failed to "
                      "create or something registered a duplicate state");
        CHECK_MESSAGE(samplers.Get(Shift::GPU::SAMPLER_ANISO_REPEAT) != nullptr,
                      "the slot a shader would index with SAMPLER_ANISO_REPEAT holds no sampler");

        //! The viewport sampler already exercised this at boot: it asks for nearest/repeat under a
        //! different debug name and must land on the named slot rather than spend a fifth one
        Shift::SamplerDescriptor repeated{};
        repeated.minFilter = Shift::EFilterMode::Nearest;
        repeated.magFilter = Shift::EFilterMode::Nearest;
        repeated.name = "ADifferentDebugName";
        const uint32_t deduped = samplers.GetOrCreate(repeated);
        CHECK_MESSAGE(deduped == Shift::GPU::SAMPLER_NEAREST_REPEAT,
                      "an identical sampler state took a second slot - the debug name is splitting the cache");
        CHECK(samplers.GetCount() == namedCount);

        //! ...and a state nobody registered at boot is reachable mid-run, which is why b0 is
        //! update-after-bind: frames have already run by now, so this writes a descriptor into a
        //! set that in-flight command buffers have genuinely bound
        Shift::SamplerDescriptor nearestClamp{};
        nearestClamp.minFilter = Shift::EFilterMode::Nearest;
        nearestClamp.magFilter = Shift::EFilterMode::Nearest;
        nearestClamp.addressModeU = Shift::ESamplerAddressMode::ClampEdge;
        nearestClamp.addressModeV = Shift::ESamplerAddressMode::ClampEdge;
        nearestClamp.addressModeW = Shift::ESamplerAddressMode::ClampEdge;
        nearestClamp.name = "NearestClamp";
        const uint32_t fresh = samplers.GetOrCreate(nearestClamp);
        CHECK_MESSAGE(fresh == namedCount, "a new sampler state did not take the next free slot");
        CHECK(samplers.Get(fresh) != nullptr);
        CHECK(samplers.GetCount() == namedCount + 1u);
    }
    tick(3);

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

    renderer.ResizeViewport(800, 400);
    tick(3);

    const Shift::GPU::FrameConstants* wideFrame = renderer.GetFrameConstants(0);
    REQUIRE_MESSAGE(wideFrame != nullptr, "frame slot 0 has no frame constants");
    const float wideProjX = wideFrame->proj[0][0];

    for (uint32_t slot = 0; slot < Shift::Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++slot) {
        CAPTURE(slot);
        const Shift::GPU::FrameConstants* frame = renderer.GetFrameConstants(slot);
        REQUIRE_MESSAGE(frame != nullptr, "in-flight slot has no frame constants");

        //! Several frames have gone by, so every slot must have been written. A slot still holding
        //! the zeroed initial state means the renderer writes one slot for every frame instead of
        //! the frame's own - which would be invisible on screen and a race under overlap
        const bool viewProjWritten = MaxAbs(frame->viewProj) > 0.0f;
        CHECK_MESSAGE(viewProjWritten,
                      "frame slot was never written: the ring is not being indexed per frame");

        //! The shader multiplies by viewProj alone, so it has to agree with the view and proj it
        //! was built from - this is what catches a half-updated or torn write
        CHECK(MaxAbsDiff(frame->viewProj, frame->proj * frame->view) < 1e-4f);

        //! The pull-model stream addresses reached the struct the shader reads. Zero here means the
        //! vertex shader is dereferencing a null pointer for every vertex
        CHECK_MESSAGE(frame->positionsRef != 0, "position stream address never reached the GPU struct");
        CHECK_MESSAGE(frame->normalsRef != 0, "normal stream address never reached the GPU struct");
        CHECK_MESSAGE(frame->tangentsRef != 0, "tangent stream address never reached the GPU struct");
        CHECK_MESSAGE(frame->uvsRef != 0, "uv stream address never reached the GPU struct");

        //! Four SEPARATE merged buffers. One address repeated would mean two streams were created
        //! from the same handle, and every normal would read as a position
        //! All SIX pairs, not a cycle of four: positions aliasing tangents is just as wrong as
        //! positions aliasing normals, and a chain of neighbour comparisons misses it
        std::vector<uint64_t> streamRefs{
            frame->positionsRef, frame->normalsRef, frame->tangentsRef, frame->uvsRef
        };
        std::sort(streamRefs.begin(), streamRefs.end());
        const bool streamsDistinct = std::unique(streamRefs.begin(), streamRefs.end()) == streamRefs.end();
        CHECK_MESSAGE(streamsDistinct, "two SoA vertex streams resolve to the same address");

        //! Each frame in flight hands the shader ITS OWN ObjectData slot - that is the whole reason
        //! the array is a per-frame ring rather than one shared buffer
        CHECK_MESSAGE(frame->objectBufferRef != 0, "object array address never reached the GPU struct");
    }

    {
        const Shift::GPU::FrameConstants* slot0 = renderer.GetFrameConstants(0);
        const Shift::GPU::FrameConstants* slot1 = renderer.GetFrameConstants(1);
        REQUIRE(slot0 != nullptr);
        REQUIRE(slot1 != nullptr);
        CHECK_MESSAGE(slot0->objectBufferRef != slot1->objectBufferRef,
                      "two frames in flight were handed the SAME ObjectData address, so one frame's "
                      "CPU rewrite lands in an array the other is still reading");
    }

    //! The merged scene geometry, the extracted frame, and the one number the pull model
    //! cannot work without. Keyed on the committed asset only: DamagedHelmet is downloaded (R7)
    CheckExtractedFrame(renderer);

    //! Projection tracks the VIEWPORT render target's aspect, not the OS window's: the scene is
    //! drawn into the viewport texture, and the editor is free to give it any shape
    renderer.ResizeViewport(400, 800);
    tick(3);

    const Shift::GPU::FrameConstants* tallFrame = renderer.GetFrameConstants(0);
    REQUIRE_MESSAGE(tallFrame != nullptr, "frame slot 0 has no frame constants");
    CHECK_MESSAGE(std::fabs(tallFrame->proj[0][0] - wideProjX) > 1e-3f,
                  "projection did not change when the viewport aspect flipped from 2:1 to 1:2, so "
                  "either the camera is not tracking the render target or the ring is stale");

    {
        const Shift::GPU::FrameConstants* before = renderer.GetFrameConstants(0);
        REQUIRE(before != nullptr);
        const glm::mat4 viewBefore = before->view;

        //! Exactly what MouseButtonCallback/KeyCallback do when you hold RMB and press W
        Shift::inp::Keyboard::GetInstance().SetKeyAction(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
        Shift::inp::Keyboard::GetInstance().SetKeyAction(GLFW_KEY_W, GLFW_PRESS);
        tick(5);
        Shift::inp::Keyboard::GetInstance().SetKeyAction(GLFW_KEY_W, GLFW_RELEASE);
        Shift::inp::Keyboard::GetInstance().SetKeyAction(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
        tick(2);

        const Shift::GPU::FrameConstants* after = renderer.GetFrameConstants(0);
        REQUIRE(after != nullptr);
        CHECK_MESSAGE(MaxAbsDiff(viewBefore, after->view) > 1e-4f,
                      "holding RMB + W did not move the camera: either the controller never ran, "
                      "or the camera's movement never reached the frame constants");
    }

    //! Phase 5: shader hot-reload (watcher deliberately bypassed: MarkDirty is used)
    //! Real recompile, in-place pipeline rebuild, retired GPU handle released via the deferred
    //! executor once the timeline passes. Each Lib module is asked SEPARATELY because each enters
    //! through a different import chain, and a dependency list reaching one says nothing about
    //! another: ForwardVS imports FrameData + VertexPull, ForwardPS imports FrameData + Bindless
    const char* dirtiedShaders[] = {
        "Forward/ForwardVS.slang",
        "Lib/FrameData.slang",
        "Lib/Bindless.slang",
        "Lib/VertexPull.slang",
    };
    for (const char* shader : dirtiedShaders) {
        CAPTURE(shader);
        renderer.GetShaderManager().MarkDirty(Shift::Util::GetShiftShaderSrcDir() + shader);
        CHECK_MESSAGE(renderer.HotReloadShaders() == 1,
                      "marking this dirty rebuilt no pipeline: the forward shaders' dependency "
                      "lists do not reach through the import");
        tick(5);
    }

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
    //! The scene pass - and with it the depth attachment, the index binding and every draw - only
    //! exists inside ViewportPass. If this only ever ran on frame 0 then the run says nothing about
    //! the draw path, and nothing about how two OVERLAPPING frames share one depth buffer
    CHECK_MESSAGE(viewportPassFrames > 1,
                  "the scene pass ran on at most one frame, so this run did not exercise drawing "
                  "or the depth attachment across frames");

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
