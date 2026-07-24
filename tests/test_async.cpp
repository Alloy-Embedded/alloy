// Unit tests for the heap-less coroutine runtime (src/alloy/async/*). Exercises
// the exact suspend/resume + ISR-wake flow that runs on silicon, driven by a
// simulated interrupt (event::set), plus the regressions an adversarial review
// surfaced: the lost-wakeup race (signal in the await_ready/await_suspend
// window) and retire-releases-storage. Compiled -fno-exceptions/-fno-rtti +
// sanitizers, the same code that cross-compiles to the MCU.

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>

#include "alloy/async/event.hpp"
#include "alloy/async/executor.hpp"
#include "alloy/async/delay.hpp"
#include "alloy/async/task.hpp"
#include "alloy/async/uart.hpp"
#include "alloy_test.hpp"

using namespace alloy::async;
using namespace std::chrono_literals;

// Test-only virtual clock hooks (defined in host_support.cpp).
namespace alloy::test {
void set_uptime_ms(std::uint32_t ms);
void advance_uptime_ms(std::uint32_t d);
}  // namespace alloy::test

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

ALLOY_TEST(async_double_spawn_is_idempotent) {
    // spawn() twice on one task (or any double-enqueue) must NOT queue it twice —
    // resuming a coroutine that is already queued/running is UB. The executor's
    // per-task queued flag makes schedule() idempotent.
    executor<8> ex;
    task_storage<256> st;
    event ev;
    int count = 0;

    task t = counter_task(st, ev, count, 1);
    ex.spawn(t);
    ex.spawn(t);  // double-spawn
    ALLOY_CHECK_EQ(ex.ready_count(), static_cast<std::size_t>(1));  // enqueued once

    ex.run_once();  // parks on co_await ev
    ALLOY_CHECK_EQ(count, 0);
    ev.set();
    ex.run_once();
    ALLOY_CHECK_EQ(count, 1);  // ran exactly once, not twice
}

namespace {
task park_on(task_storage<256>&, event& ev) { co_await ev; }
}  // namespace

ALLOY_TEST(async_second_waiter_on_one_event_traps) {
    // Single-owner: two tasks parking on ONE event is a design error and must
    // trap loudly (not silently starve one). Verified as a death test — the
    // child must NOT exit cleanly.
    const pid_t pid = fork();
    if (pid == 0) {
        executor<8> ex;
        task_storage<256> sa;
        task_storage<256> sb;
        event ev;
        ex.spawn(park_on(sa, ev));
        ex.spawn(park_on(sb, ev));
        ex.run_once();  // A parks on ev; B's park() finds ev owned -> traps
        _exit(0);        // reached only if the guard FAILED to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    ALLOY_CHECK(!(WIFEXITED(status) && WEXITSTATUS(status) == 0));
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

// ---- sleep(ms) ----

namespace {
task sleeper_task(task_storage<256>&, int& ticks) {
    for (int i = 0; i < 3; ++i) {
        co_await delay(100ms);
        ++ticks;
    }
}
}  // namespace

ALLOY_TEST(async_sleep_wakes_when_the_clock_passes_the_deadline) {
    alloy::test::set_uptime_ms(0);
    executor<8> ex;
    task_storage<256> st;
    int ticks = 0;

    task t = sleeper_task(st, ticks);
    ex.spawn(t);
    ex.run_once();  // start -> arms a 100ms timer, parks
    ALLOY_CHECK_EQ(ticks, 0);

    ex.run_once();  // clock still 0 -> not due
    ALLOY_CHECK_EQ(ticks, 0);

    alloy::test::advance_uptime_ms(100);  // reach the deadline
    ex.run_once();
    ALLOY_CHECK_EQ(ticks, 1);  // woke, then armed the next 100ms

    alloy::test::advance_uptime_ms(99);  // one short
    ex.run_once();
    ALLOY_CHECK_EQ(ticks, 1);

    alloy::test::advance_uptime_ms(1);  // now due
    ex.run_once();
    ALLOY_CHECK_EQ(ticks, 2);

    alloy::test::advance_uptime_ms(100);
    ex.run_once();
    ALLOY_CHECK_EQ(ticks, 3);  // third sleep done -> task retired
    ALLOY_CHECK(!st.in_use);
}

// ---- async uart.read() ----

namespace {
// A fake UART matching the on_receive contract; feed() simulates the RX ISR.
struct mock_uart {
    void (*fn)(void*, std::uint8_t) = nullptr;
    void* ctx = nullptr;
    void on_receive(void (*f)(void*, std::uint8_t), void* c) {
        fn = f;
        ctx = c;
    }
    void feed(std::uint8_t b) {
        if (fn) {
            fn(ctx, b);
        }
    }
};

task echo_collect(task_storage<256>&, uart_reader<mock_uart>& rx, char* out, int n) {
    for (int i = 0; i < n; ++i) {
        out[i] = static_cast<char>(co_await rx.read());
    }
}
}  // namespace

ALLOY_TEST(async_uart_read_delivers_bytes_across_suspensions) {
    executor<8> ex;
    task_storage<256> st;
    mock_uart mu;
    uart_reader<mock_uart> rx{mu};
    char buf[4] = {};

    task t = echo_collect(st, rx, buf, 3);
    ex.spawn(t);
    ex.run_once();  // parks on the first read (ring empty)

    mu.feed('H');  // "interrupt"
    ex.run_once();
    mu.feed('i');
    ex.run_once();
    mu.feed('!');
    ex.run_once();

    ALLOY_CHECK_EQ(buf[0], 'H');
    ALLOY_CHECK_EQ(buf[1], 'i');
    ALLOY_CHECK_EQ(buf[2], '!');
    ALLOY_CHECK(!st.in_use);  // collected 3 -> retired
}

ALLOY_TEST(async_uart_buffers_a_burst_that_arrives_while_busy) {
    // Bytes that land before the task drains must be buffered by the SPSC ring,
    // not lost (the "two bytes, one slot" regression). Feed a burst, THEN run.
    executor<8> ex;
    task_storage<256> st;
    mock_uart mu;
    uart_reader<mock_uart> rx{mu};
    char buf[6] = {};

    task t = echo_collect(st, rx, buf, 5);
    ex.spawn(t);
    ex.run_once();  // parks

    mu.feed('a');
    mu.feed('b');
    mu.feed('c');  // three bytes buffered before any resume
    ex.run_once();  // one wake was scheduled; await_ready drains the ring greedily
    mu.feed('d');
    mu.feed('e');
    ex.run_once();

    ALLOY_CHECK_EQ(buf[0], 'a');
    ALLOY_CHECK_EQ(buf[1], 'b');
    ALLOY_CHECK_EQ(buf[2], 'c');
    ALLOY_CHECK_EQ(buf[3], 'd');
    ALLOY_CHECK_EQ(buf[4], 'e');
    ALLOY_CHECK(!st.in_use);
}
