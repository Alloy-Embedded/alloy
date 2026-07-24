// Unit tests for the heap-less coroutine runtime (src/alloy/async/*). Exercises
// the exact suspend/resume + ISR-wake flow that runs on silicon, driven by a
// simulated interrupt (event::set), plus the regressions an adversarial review
// surfaced: the lost-wakeup race (signal in the await_ready/await_suspend
// window) and retire-releases-storage. Compiled -fno-exceptions/-fno-rtti +
// sanitizers, the same code that cross-compiles to the MCU.

#include <cstdint>

#include "alloy/async/event.hpp"
#include "alloy/async/executor.hpp"
#include "alloy/async/task.hpp"
#include "alloy_test.hpp"

using namespace alloy::async;

namespace {

// Finite task: await `n` wakes then return, so it retires and frees its storage.
task counter_task(task_storage<256>&, event& ev, int& count, int n) {
    for (int i = 0; i < n; ++i) {
        co_await ev;
        ++count;
    }
}

// Frame-local `sum` must survive every suspension (it lives in the frame).
task accumulate_task(task_storage<256>&, event& ev, int& out, int n) {
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        co_await ev;
        sum += (i + 1);
        out = sum;
    }
}

}  // namespace

ALLOY_TEST(async_event_wakes_parked_task_each_time) {
    executor<8> ex;
    task_storage<256> st;
    event ev;
    int count = 0;

    task t = counter_task(st, ev, count, 3);
    ex.spawn(t);
    ex.run_once();  // start -> parks on the first co_await
    ALLOY_CHECK_EQ(count, 0);
    ALLOY_CHECK(st.in_use);

    ev.set();  // "interrupt"
    ex.run_once();
    ALLOY_CHECK_EQ(count, 1);  // resumed, then parked again

    ev.set();
    ex.run_once();
    ALLOY_CHECK_EQ(count, 2);

    ev.set();
    ex.run_once();
    ALLOY_CHECK_EQ(count, 3);
    // Third wake completed the loop -> task retired -> storage released.
    ALLOY_CHECK(!st.in_use);
}

ALLOY_TEST(async_signal_before_await_resumes_immediately) {
    // The lost-wakeup case: the signal is already latched before the task ever
    // parks. await_ready must see it and skip suspension (no hang).
    executor<8> ex;
    task_storage<256> st;
    event ev;
    int count = 0;

    ev.set();  // signal arrives first
    task t = counter_task(st, ev, count, 1);
    ex.spawn(t);
    ex.run_once();  // await_ready sees signalled -> runs body without parking
    ALLOY_CHECK_EQ(count, 1);
    ALLOY_CHECK(!st.in_use);  // single-await task already retired
}

ALLOY_TEST(async_frame_local_persists_across_suspensions) {
    executor<8> ex;
    task_storage<256> st;
    event ev;
    int out = 0;

    task t = accumulate_task(st, ev, out, 4);
    ex.spawn(t);
    ex.run_once();
    for (int i = 0; i < 4; ++i) {
        ev.set();
        ex.run_once();
    }
    ALLOY_CHECK_EQ(out, 1 + 2 + 3 + 4);  // frame-local sum survived 4 suspensions
    ALLOY_CHECK(!st.in_use);
}

ALLOY_TEST(async_storage_is_reusable_after_retire) {
    executor<8> ex;
    task_storage<256> st;
    event ev;
    int count = 0;

    task t1 = counter_task(st, ev, count, 1);
    ex.spawn(t1);
    ex.run_once();
    ev.set();
    ex.run_once();
    ALLOY_CHECK_EQ(count, 1);
    ALLOY_CHECK(!st.in_use);

    // Re-spawn a fresh task in the SAME storage — must not trap (storage free).
    task t2 = counter_task(st, ev, count, 1);
    ex.spawn(t2);
    ex.run_once();
    ev.set();
    ex.run_once();
    ALLOY_CHECK_EQ(count, 2);
    ALLOY_CHECK(!st.in_use);
}

ALLOY_TEST(async_two_independent_tasks_interleave) {
    executor<8> ex;
    task_storage<256> sa;
    task_storage<256> sb;
    event ea;
    event eb;
    int ca = 0;
    int cb = 0;

    task ta = counter_task(sa, ea, ca, 2);
    task tb = counter_task(sb, eb, cb, 2);
    ex.spawn(ta);
    ex.spawn(tb);
    ex.run_once();  // both park

    ea.set();  // wake only A
    ex.run_once();
    ALLOY_CHECK_EQ(ca, 1);
    ALLOY_CHECK_EQ(cb, 0);  // B untouched — independent events

    eb.set();
    ea.set();
    ex.run_once();
    ALLOY_CHECK_EQ(ca, 2);  // A completed (retired)
    ALLOY_CHECK_EQ(cb, 1);
    ALLOY_CHECK(!sa.in_use);

    eb.set();  // finish B too, so both retire cleanly
    ex.run_once();
    ALLOY_CHECK_EQ(cb, 2);
    ALLOY_CHECK(!sb.in_use);
}
