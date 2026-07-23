#include <doctest/doctest.h>

#include <vector>

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

//! A bucket whose frame already passed still fires on the next sweep
//! instead of leaking until shutdown
TEST_CASE("A past-frame bucket fires on a later sweep") {
    RHIDeferredExecutor executor;
    int runs = 0;

    executor.DeferExecuteToFrame(3, [&] { ++runs; });

    executor.ProcessDeferredCallbacks(4);
    CHECK(runs == 1);
}

//! Every bucket at or before the current frame drains in one sweep, ascending; future buckets stay.
TEST_CASE("Multiple overdue frame buckets fire together in ascending order") {
    RHIDeferredExecutor executor;
    std::vector<int> order;

    executor.DeferExecuteToFrame(2, [&] { order.push_back(2); });
    executor.DeferExecuteToFrame(5, [&] { order.push_back(5); });
    executor.DeferExecuteToFrame(4, [&] { order.push_back(4); });
    executor.DeferExecuteToFrame(9, [&] { order.push_back(9); }); // still in the future at frame 6

    executor.ProcessDeferredCallbacks(6);

    REQUIRE(order.size() == 3);
    CHECK(order[0] == 2);
    CHECK(order[1] == 4);
    CHECK(order[2] == 5);

    executor.ProcessDeferredCallbacks(9);
    REQUIRE(order.size() == 4);
    CHECK(order[3] == 9);
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
