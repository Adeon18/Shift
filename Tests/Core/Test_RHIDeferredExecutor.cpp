#include <doctest/doctest.h>

#include "Graphics/RHI/RHIDeferredExecutor.hpp"

using Shift::RHIDeferredExecutor;

//! CPU-side contract of the deferred executor. Timeline-gated paths need a real device
//! semaphore and are to be added later
TEST_SUITE("RHIDeferredExecutor") {

TEST_CASE("A null timeline executes the callback immediately") {
    RHIDeferredExecutor executor;
    int runs = 0;

    executor.DeferExecute(nullptr, 123, [&] { ++runs; });

    CHECK(runs == 1);
}

TEST_CASE("A frame callback fires exactly once, on its exact frame") {
    RHIDeferredExecutor executor;
    int runs = 0;

    executor.DeferExecuteToFrame(5, [&] { ++runs; });

    executor.ProcessDeferredCallbacks(4);
    CHECK(runs == 0);

    executor.ProcessDeferredCallbacks(5);
    CHECK(runs == 1);

    executor.ProcessDeferredCallbacks(5);
    CHECK(runs == 1);
}

//! F-FRAMEDEFER (fix scheduled with F-FIF, Batch C): a bucket for an already-passed frame
//! should still fire on the next sweep instead of leaking until shutdown. This asserts the
//! documented-correct behavior, which is currently known-broken — hence may_fail (reported,
//! doesn't fail the suite). Flip to a hard CHECK when F-FRAMEDEFER lands.
TEST_CASE("A past-frame bucket fires on a later sweep" * doctest::may_fail()) {
    RHIDeferredExecutor executor;
    int runs = 0;

    executor.DeferExecuteToFrame(3, [&] { ++runs; });

    executor.ProcessDeferredCallbacks(4);
    CHECK(runs == 1);
}

TEST_CASE("A callback may defer more work without deadlocking)") {
    RHIDeferredExecutor executor;
    int first = 0;
    int nested = 0;

    executor.DeferExecuteToFrame(1, [&] {
        ++first;
        executor.DeferExecuteToFrame(2, [&] { ++nested; });
    });

    //! Deadlocked right here before the fix: the callback re-entered the executor's mutex
    executor.ProcessDeferredCallbacks(1);
    CHECK(first == 1);
    CHECK(nested == 0);

    executor.ProcessDeferredCallbacks(2);
    CHECK(nested == 1);
}

TEST_CASE("Flush drains re-deferred work to a fixpoint") {
    RHIDeferredExecutor executor;
    int first = 0;
    int nested = 0;

    executor.DeferExecuteEndOfSession([&] {
        ++first;
        executor.DeferExecuteEndOfSession([&] { ++nested; });
    });

    executor.FlushAllDeferredCallbacks();

    //! Cleanup relies on "nothing pending after flush" before it starts deleting managers,
    //! so work deferred DURING the flush must run inside the same flush
    CHECK(first == 1);
    CHECK(nested == 1);
}

TEST_CASE("Flush runs every category and consumes everything") {
    RHIDeferredExecutor executor;
    int frameRuns = 0;
    int sessionRuns = 0;

    executor.DeferExecuteToFrame(99, [&] { ++frameRuns; });
    executor.DeferExecuteEndOfSession([&] { ++sessionRuns; });

    executor.FlushAllDeferredCallbacks();
    CHECK(frameRuns == 1);
    CHECK(sessionRuns == 1);

    //! A second flush must be a no-op : nothing may run twice
    executor.FlushAllDeferredCallbacks();
    CHECK(frameRuns == 1);
    CHECK(sessionRuns == 1);
}

TEST_CASE("An empty callback is rejected without crashing") {
    RHIDeferredExecutor executor;

    executor.DeferExecuteToFrame(7, {});
    executor.ProcessDeferredCallbacks(7);

    executor.DeferExecute(nullptr, 0, {});

    CHECK(true);
}

}
