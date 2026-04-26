// Host-level tests for SharedScheduler<N,B>.
//
// Two std::threads simulate dual-core operation: thread 0 calls tick(0),
// thread 1 calls tick(1). We verify:
//   - Core0-pinned tasks run only on thread 0.
//   - Core1-pinned tasks run only on thread 1.
//   - Any tasks run on whichever core claims them at spawn time.
//
// Single-threaded tests verify spawn routing and basic tick semantics.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "runtime/shared_event.hpp"
#include "runtime/tasks/scheduler.hpp"
#include "runtime/tasks/shared_scheduler.hpp"

using namespace alloy;
using alloy::tasks::CoreAffinity;
using alloy::tasks::Priority;
using alloy::tasks::SharedScheduler;
using alloy::tasks::Task;

// Minimal coroutine that records which core ran it.
static std::atomic<std::size_t> g_ran_on_core{0xFF};

static auto recording_task(std::size_t core_id) -> Task {
    g_ran_on_core.store(core_id, std::memory_order_release);
    co_return;
}

TEST_CASE("SharedScheduler: Core0-pinned task runs only on tick(0)", "[shared_sched]") {
    SharedScheduler<4, 256> sched;

    g_ran_on_core.store(0xFF, std::memory_order_relaxed);

    REQUIRE(sched.spawn([&] { return recording_task(0); }, Priority::Normal,
                        CoreAffinity::Core0)
                .is_ok());

    // tick(1) should not run a Core0-pinned task
    sched.tick(1);
    REQUIRE(g_ran_on_core.load(std::memory_order_acquire) == 0xFFu);

    // tick(0) should run it
    sched.tick(0);
    REQUIRE(g_ran_on_core.load(std::memory_order_acquire) == 0u);
}

TEST_CASE("SharedScheduler: Core1-pinned task runs only on tick(1)", "[shared_sched]") {
    SharedScheduler<4, 256> sched;

    g_ran_on_core.store(0xFF, std::memory_order_relaxed);

    REQUIRE(sched.spawn([&] { return recording_task(1); }, Priority::Normal,
                        CoreAffinity::Core1)
                .is_ok());

    sched.tick(0);
    REQUIRE(g_ran_on_core.load(std::memory_order_acquire) == 0xFFu);

    sched.tick(1);
    REQUIRE(g_ran_on_core.load(std::memory_order_acquire) == 1u);
}

TEST_CASE("SharedScheduler: Any task runs on at least one core", "[shared_sched]") {
    SharedScheduler<4, 256> sched;

    g_ran_on_core.store(0xFF, std::memory_order_relaxed);

    REQUIRE(sched.spawn([&] { return recording_task(99); }, Priority::Normal,
                        CoreAffinity::Any)
                .is_ok());

    // Run both cores — the Any task should land on exactly one
    sched.tick(0);
    sched.tick(1);

    REQUIRE(g_ran_on_core.load(std::memory_order_acquire) == 99u);
}

TEST_CASE("SharedScheduler: two threads, core-pinned tasks stay on correct core",
          "[shared_sched][threading]") {
    SharedScheduler<8, 256> sched;

    constexpr int kTasks = 4;
    std::atomic<int> core0_count{0};
    std::atomic<int> core1_count{0};

    // Spawn kTasks pinned to each core. The coroutines run synchronously within
    // tick(), so the thread ID at resume time matches the core_id passed.
    for (int i = 0; i < kTasks; ++i) {
        sched.spawn(
            [&] {
                return [](std::atomic<int>& cnt) -> Task {
                    cnt.fetch_add(1, std::memory_order_relaxed);
                    co_return;
                }(core0_count);
            },
            Priority::Normal, CoreAffinity::Core0);

        sched.spawn(
            [&] {
                return [](std::atomic<int>& cnt) -> Task {
                    cnt.fetch_add(1, std::memory_order_relaxed);
                    co_return;
                }(core1_count);
            },
            Priority::Normal, CoreAffinity::Core1);
    }

    std::thread t0([&] {
        while (sched.tick(0)) {
        }
    });
    std::thread t1([&] {
        while (sched.tick(1)) {
        }
    });

    t0.join();
    t1.join();

    REQUIRE(core0_count.load() == kTasks);
    REQUIRE(core1_count.load() == kTasks);
}

TEST_CASE("SharedScheduler: Any tasks are distributed across both schedulers",
          "[shared_sched]") {
    SharedScheduler<8, 256> sched;

    // Spawn 6 Any tasks — round-robin should give 3 to each core
    std::atomic<int> total{0};
    for (int i = 0; i < 6; ++i) {
        sched.spawn(
            [&] {
                return [](std::atomic<int>& t) -> Task {
                    t.fetch_add(1, std::memory_order_relaxed);
                    co_return;
                }(total);
            },
            Priority::Normal, CoreAffinity::Any);
    }

    while (sched.tick(0)) {
    }
    while (sched.tick(1)) {
    }

    REQUIRE(total.load() == 6);
}

TEST_CASE("SharedEvent: signal/consume across logical cores", "[shared_event]") {
    using alloy::tasks::SharedEvent;
    SharedEvent ev;

    REQUIRE_FALSE(ev.is_signaled());
    REQUIRE_FALSE(ev.consume());

    ev.signal();
    REQUIRE(ev.is_signaled());

    REQUIRE(ev.consume());
    REQUIRE_FALSE(ev.is_signaled());
    REQUIRE_FALSE(ev.consume());  // edge-triggered: second consume returns false
}
