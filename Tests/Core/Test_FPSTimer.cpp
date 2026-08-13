//! CPU-only. The frame timer feeds dt to everything time-based - camera movement, animation,
//! anything that integrates. It is also the one component the [app] smoke deliberately bypasses
//! (tests inject a fixed dt for determinism), so nothing else in the suite exercises it.
//!
//! The shape that matters is how ShiftEngine::Run uses it: HasFrameElapsed() is polled in a tight
//! spin loop, and the dt handed to Tick must be the time since the previous FRAME, not the time
//! since the previous poll. Those differ by three orders of magnitude when the app runs faster
//! than the frame cap, which is the normal case for a light scene.
#include <doctest/doctest.h>

#include "Tools/Timer/FPSTimer.hpp"

TEST_SUITE("FPSTimer") {

TEST_CASE("dt measures the frame, not the poll that noticed it") {
    constexpr float TARGET_FPS = 200.0f;
    constexpr float FRAME_SECONDS = 1.0f / TARGET_FPS;

    Shift::tool::FPSTimer timer{TARGET_FPS};

    //! Exactly ShiftEngine::Run's loop: poll as fast as possible, act when a frame is due.
    //! The bound only stops a broken timer from hanging the suite - it is deliberately NOT a
    //! doctest macro, because asserting once per spin iteration would add tens of thousands of
    //! assertions to the run
    constexpr int POLL_LIMIT = 100'000'000;
    int polls = 0;
    while (!timer.HasFrameElapsed()) {
        if (++polls >= POLL_LIMIT) { break; }
    }
    REQUIRE_MESSAGE(polls < POLL_LIMIT, "the timer never reported an elapsed frame");

    //! A frame is only reported once the accumulated lag passes the cap, so the elapsed frame time
    //! cannot legitimately be below it. Reporting the spin interval instead yields microseconds -
    //! a thousand times smaller - and every dt-scaled motion in the engine stalls
    const float dt = timer.GetDt();
    CAPTURE(polls);
    CAPTURE(dt);
    CHECK_MESSAGE(dt >= FRAME_SECONDS * 0.9f,
                  "dt is the interval between two polls of the spin loop rather than the frame's "
                  "own duration, so everything scaled by dt effectively stops");

    //! Same number in the other unit, since EngineData carries both
    CHECK(timer.GetFrameTimeInMs() >= FRAME_SECONDS * 0.9f * 1000.0f);

    //! Sanity: a frame at a 200 FPS cap is milliseconds, not seconds. Loose on purpose - this is
    //! wall-clock, and the point is the order of magnitude, not the exact value
    CHECK(dt < 1.0f);
}

}
