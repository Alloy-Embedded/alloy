// Unit tests for the heap-less coroutine runtime (src/alloy/async/*). Exercises
// the exact suspend/resume + ISR-wake flow that runs on silicon, driven by a
// simulated interrupt (event::set), plus the regressions an adversarial review
// surfaced: the lost-wakeup race (signal in the await_ready/await_suspend
// window) and retire-releases-storage. Compiled -fno-exceptions/-fno-rtti +
// sanitizers, the same code that cross-compiles to the MCU.

#include <sys/wait.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "alloy/async/event.hpp"
#include "alloy/async/executor.hpp"
#include "alloy/async/periodic.hpp"
#include "alloy/async/delay.hpp"
#include "alloy/async/dma.hpp"
#include "alloy/async/i2c.hpp"
#include "alloy/async/spi.hpp"
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

// ---- driver awaitables: dma / spi / i2c ----
//
// These three share one shape (see src/alloy/async/{dma,spi,i2c}.hpp): the
// operation is STARTED INSIDE await_suspend, after the task is parked, so the
// suspend is unconditional and the completion interrupt has no window to race
// into. Two properties are worth testing per awaitable and are tested below:
//   (a) the task really suspends and is resumed by the simulated ISR — the
//       run_once() RETURN VALUE is the observable (1 resume = parked, still
//       alive; a second resume only happens because the callback woke it), and
//   (b) a completion delivered DURING the start (a peripheral that finishes
//       before its start call returns — the ordinary case under emulation) is
//       not lost. Start-then-park would drop that wake and hang forever.

namespace {

// A fake SPI matching alloy::spi::handle's interrupt-driven contract.
// complete() plays the driver's ISR: fill rx, clear busy, call back.
struct mock_spi {
    void (*fn)(void*) = nullptr;
    void* ctx = nullptr;
    std::uint8_t* rx = nullptr;
    const std::uint8_t* answer = nullptr;
    std::size_t len = 0;
    int starts = 0;
    bool busy_ = false;
    bool complete_inside_start = false;  // "finished before the start returned"

    void transfer_async(std::span<const std::uint8_t> t, std::span<std::uint8_t> r,
                        void (*f)(void*), void* c) {
        rx = r.data();
        len = t.empty() ? r.size() : t.size();
        fn = f;
        ctx = c;
        busy_ = true;
        ++starts;
        if (complete_inside_start) {
            complete();
        }
    }
    [[nodiscard]] bool busy() const { return busy_; }
    void complete() {
        if (rx != nullptr && answer != nullptr) {
            for (std::size_t i = 0; i < len; ++i) {
                rx[i] = answer[i];
            }
        }
        busy_ = false;
        if (fn != nullptr) {
            fn(ctx);
        }
    }
};

task spi_once(task_storage<256>&, spi_master<mock_spi>& m, std::span<const std::uint8_t> out,
              std::span<std::uint8_t> in) {
    co_await m.transfer(out, in);
}

}  // namespace

ALLOY_TEST(async_spi_transfer_parks_then_resumes_from_the_completion_interrupt) {
    executor<8> ex;
    task_storage<256> st;
    mock_spi ms;
    const std::uint8_t answer[3] = {0xDE, 0xAD, 0xBE};
    ms.answer = answer;
    spi_master<mock_spi> m{ms};

    static const std::uint8_t out[3] = {1, 2, 3};
    std::uint8_t in[3] = {};
    task t = spi_once(st, m, out, in);
    ex.spawn(t);

    // One resume, and the task is still alive: it parked inside the co_await.
    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{1});
    ALLOY_CHECK(st.in_use);
    ALLOY_CHECK_EQ(ms.starts, 1);   // ...and the transfer was started by the await
    ALLOY_CHECK(ms.busy());

    ms.complete();  // the completion "interrupt"
    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{1});
    ALLOY_CHECK(!st.in_use);  // retired -> storage released
    ALLOY_CHECK_EQ(in[0], std::uint8_t{0xDE});
    ALLOY_CHECK_EQ(in[1], std::uint8_t{0xAD});
    ALLOY_CHECK_EQ(in[2], std::uint8_t{0xBE});
}

ALLOY_TEST(async_spi_completion_inside_the_start_is_not_lost) {
    // The regression the park-before-start ordering exists for: a peripheral
    // that completes before transfer_async() returns. The waiter is already
    // registered, so the wake lands on the ready queue and the SAME superstep
    // resumes the task. With start-then-park the wake would have nowhere to go.
    executor<8> ex;
    task_storage<256> st;
    mock_spi ms;
    const std::uint8_t answer[2] = {0x5A, 0xA5};
    ms.answer = answer;
    ms.complete_inside_start = true;
    spi_master<mock_spi> m{ms};

    static const std::uint8_t out[2] = {9, 9};
    std::uint8_t in[2] = {};
    task t = spi_once(st, m, out, in);
    ex.spawn(t);

    // Two resumes in one superstep: the start, and the wake it delivered.
    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{2});
    ALLOY_CHECK(!st.in_use);
    ALLOY_CHECK_EQ(in[0], std::uint8_t{0x5A});
    ALLOY_CHECK_EQ(in[1], std::uint8_t{0xA5});
}

namespace {

// A fake I2C matching alloy::i2c::handle's interrupt-driven contract.
struct mock_i2c {
    void (*fn)(void*) = nullptr;
    void* ctx = nullptr;
    std::uint8_t* rx = nullptr;
    const std::uint8_t* answer = nullptr;
    std::size_t len = 0;
    std::uint8_t last_addr = 0;
    std::uint8_t sent[4] = {};
    int writes = 0;
    int reads = 0;
    bool busy_ = false;
    bool ok_ = false;
    bool nack = false;  // the next transfer is NACKed

    void write_async(std::uint8_t addr, std::span<const std::uint8_t> d, void (*f)(void*),
                     void* c) {
        last_addr = addr;
        for (std::size_t i = 0; i < d.size() && i < sizeof sent; ++i) {
            sent[i] = d[i];
        }
        rx = nullptr;
        len = d.size();
        fn = f;
        ctx = c;
        busy_ = true;
        ++writes;
    }
    void read_async(std::uint8_t addr, std::span<std::uint8_t> d, void (*f)(void*), void* c) {
        last_addr = addr;
        rx = d.data();
        len = d.size();
        fn = f;
        ctx = c;
        busy_ = true;
        ++reads;
    }
    [[nodiscard]] bool busy() const { return busy_; }
    [[nodiscard]] bool transfer_ok() const { return ok_; }
    void complete() {  // the STOP "interrupt"
        if (rx != nullptr && answer != nullptr) {
            for (std::size_t i = 0; i < len; ++i) {
                rx[i] = answer[i];
            }
        }
        ok_ = !nack;
        busy_ = false;
        if (fn != nullptr) {
            fn(ctx);
        }
    }
};

task i2c_write_then_read(task_storage<256>&, i2c_master<mock_i2c>& m, std::uint8_t addr,
                         std::span<const std::uint8_t> out, std::span<std::uint8_t> in,
                         bool& wrote, bool& read) {
    wrote = co_await m.write(addr, out);
    read = co_await m.read(addr, in);
}

}  // namespace

ALLOY_TEST(async_i2c_write_then_read_across_two_suspensions) {
    executor<8> ex;
    task_storage<256> st;
    mock_i2c mi;
    const std::uint8_t answer[2] = {0x12, 0x34};
    mi.answer = answer;
    i2c_master<mock_i2c> m{mi};

    static const std::uint8_t out[2] = {0xAB, 0xCD};
    std::uint8_t in[2] = {};
    bool wrote = false;
    bool read = false;
    task t = i2c_write_then_read(st, m, 0x08, out, in, wrote, read);
    ex.spawn(t);

    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{1});  // parked in the write
    ALLOY_CHECK_EQ(mi.writes, 1);
    ALLOY_CHECK_EQ(mi.reads, 0);
    ALLOY_CHECK_EQ(mi.last_addr, std::uint8_t{0x08});
    ALLOY_CHECK_EQ(mi.sent[0], std::uint8_t{0xAB});

    mi.complete();
    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{1});  // resumed, then parked in the read
    ALLOY_CHECK(wrote);
    ALLOY_CHECK_EQ(mi.reads, 1);
    ALLOY_CHECK(st.in_use);

    mi.complete();
    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{1});
    ALLOY_CHECK(read);
    ALLOY_CHECK_EQ(in[0], std::uint8_t{0x12});
    ALLOY_CHECK_EQ(in[1], std::uint8_t{0x34});
    ALLOY_CHECK(!st.in_use);
}

ALLOY_TEST(async_i2c_nack_resumes_the_task_with_false) {
    // Failure must come back through the co_await, not through a hang: the
    // driver latches ok_=false and still calls back on the STOP.
    executor<8> ex;
    task_storage<256> st;
    mock_i2c mi;
    mi.nack = true;
    i2c_master<mock_i2c> m{mi};

    static const std::uint8_t out[1] = {0x00};
    std::uint8_t in[1] = {};
    bool wrote = true;
    bool read = true;
    task t = i2c_write_then_read(st, m, 0x08, out, in, wrote, read);
    ex.spawn(t);
    ex.run_once();
    mi.complete();
    ex.run_once();
    ALLOY_CHECK(!wrote);  // the NACK surfaced as the co_await's value
    mi.complete();
    ex.run_once();
    ALLOY_CHECK(!read);
    ALLOY_CHECK(!st.in_use);
}

namespace {

// A fake DMA channel matching alloy::dma::channel's completion contract.
struct mock_dma {
    void (*fn)(void*) = nullptr;
    void* ctx = nullptr;
    bool done_ = false;
    int starts = 0;
    bool complete_inside_start = false;

    void on_complete(void (*f)(void*), void* c) {
        fn = f;
        ctx = c;
    }
    [[nodiscard]] bool done() const { return done_; }
    void start() {  // stands in for start_m2p_u8 & friends
        ++starts;
        done_ = false;
        if (complete_inside_start) {
            fire();
        }
    }
    void fire() {  // the channel's completion "interrupt"
        done_ = true;
        if (fn != nullptr) {
            fn(ctx);
        }
    }
};

task dma_once(task_storage<256>&, dma_waiter<mock_dma>& w, mock_dma& ch, int& stage) {
    stage = 1;
    co_await w.run([&] { ch.start(); });
    stage = 2;
}

}  // namespace

ALLOY_TEST(async_dma_run_parks_then_resumes_from_the_completion_interrupt) {
    executor<8> ex;
    task_storage<256> st;
    mock_dma ch;
    dma_waiter<mock_dma> w{ch};
    ALLOY_CHECK(ch.fn != nullptr);  // registered in the CTOR, before any transfer

    int stage = 0;
    task t = dma_once(st, w, ch, stage);
    ex.spawn(t);

    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{1});
    ALLOY_CHECK_EQ(stage, 1);   // suspended inside the co_await, not past it
    ALLOY_CHECK_EQ(ch.starts, 1);
    ALLOY_CHECK(st.in_use);

    ch.fire();
    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{1});
    ALLOY_CHECK_EQ(stage, 2);
    ALLOY_CHECK(!st.in_use);
}

ALLOY_TEST(async_dma_completion_inside_the_start_is_not_lost) {
    executor<8> ex;
    task_storage<256> st;
    mock_dma ch;
    ch.complete_inside_start = true;
    dma_waiter<mock_dma> w{ch};

    int stage = 0;
    task t = dma_once(st, w, ch, stage);
    ex.spawn(t);
    ALLOY_CHECK_EQ(ex.run_once(), std::size_t{2});
    ALLOY_CHECK_EQ(stage, 2);
    ALLOY_CHECK(!st.in_use);
}

// ═══════════════════ the resume budget (external arbitration) ══════════════

ALLOY_TEST(async_budgeted_run_once_resumes_at_most_the_budget) {
    executor<8> ex;
    task_storage<256> s1, s2, s3;
    event e1, e2, e3;
    int c1 = 0, c2 = 0, c3 = 0;
    task t1 = counter_task(s1, e1, c1, 1);
    task t2 = counter_task(s2, e2, c2, 1);
    task t3 = counter_task(s3, e3, c3, 1);
    ex.spawn(t1);
    ex.spawn(t2);
    ex.spawn(t3);
    ALLOY_CHECK_EQ(ex.ready_count(), 3u);

    // One decision per call: the return says what ran, ready_count() says
    // what a budgeted call left behind — together they distinguish
    // "resumed 1, drained" from "resumed 1, two still waiting".
    ALLOY_CHECK_EQ(ex.run_once(1), 1u);
    ALLOY_CHECK_EQ(ex.ready_count(), 2u);
    ALLOY_CHECK_EQ(ex.run_once(1), 1u);
    ALLOY_CHECK_EQ(ex.ready_count(), 1u);
    ALLOY_CHECK_EQ(ex.run_once(), 1u);  // unbounded drains the rest
    ALLOY_CHECK_EQ(ex.ready_count(), 0u);

    // The three parked on their first co_await, in spawn order.
    e1.set();
    e2.set();
    e3.set();
    ALLOY_CHECK_EQ(ex.run_once(2), 2u);  // budget 2 of 3
    ALLOY_CHECK_EQ(c1 + c2 + c3, 2);
    ALLOY_CHECK_EQ(ex.run_once(), 1u);
    ALLOY_CHECK_EQ(c1 + c2 + c3, 3);
}

ALLOY_TEST(async_budget_zero_wakes_timers_but_resumes_nobody) {
    executor<8> ex;
    task_storage<256> st;
    int after = 0;
    alloy::test::set_uptime_ms(1000);
    task t = sleeper_task(st, after);  // co_await delay(100ms), three times
    ex.spawn(t);
    ALLOY_CHECK_EQ(ex.run_once(), 1u);  // start; parks on the delay
    ALLOY_CHECK_EQ(ex.ready_count(), 0u);

    alloy::test::advance_uptime_ms(110);
    // Budget 0 is "poll the clock, resume nobody": the elapsed timer moves
    // its task to READY where an arbiter can see it, and nothing runs.
    ALLOY_CHECK_EQ(ex.run_once(0), 0u);
    ALLOY_CHECK_EQ(ex.ready_count(), 1u);
    ALLOY_CHECK_EQ(after, 0);
    ALLOY_CHECK_EQ(ex.run_once(1), 1u);
    ALLOY_CHECK_EQ(after, 1);
}

// ═══════════════════════ the executor observer seam ════════════════════════

namespace {
struct resume_recorder {
    std::vector<void*> begins;
    std::vector<void*> ends;
    void resume_begin(void* task) { begins.push_back(task); }
    void resume_end(void* task) { ends.push_back(task); }
};
}  // namespace

ALLOY_TEST(async_observer_brackets_each_resume_with_the_frame_identity) {
    executor<8, resume_recorder> ex;
    resume_recorder obs;
    ex.set_observer(obs);
    task_storage<256> st;
    event ev;
    int count = 0;
    task t = counter_task(st, ev, count, 2);
    ex.spawn(t);

    ALLOY_CHECK_EQ(ex.run_once(), 1u);  // the start resume
    ALLOY_CHECK_EQ(obs.begins.size(), 1u);
    ALLOY_CHECK_EQ(obs.ends.size(), 1u);
    ALLOY_CHECK(obs.begins[0] == obs.ends[0]);  // bracketed, same frame

    ev.set();
    ALLOY_CHECK_EQ(ex.run_once(), 1u);
    ALLOY_CHECK_EQ(obs.begins.size(), 2u);
    // The identity is STABLE across resumes of one task: what a name map
    // keys on.
    ALLOY_CHECK(obs.begins[1] == obs.begins[0]);
}

// ═══════════════════════ the drift-free cadence ════════════════════════════

namespace {
task metronome_task(task_storage<512>&, periodic& p, std::vector<std::uint32_t>& stamps,
                    int n) {
    for (int i = 0; i < n; ++i) {
        co_await p.next();
        stamps.push_back(alloy::uptime_ms());
    }
}
}  // namespace

ALLOY_TEST(async_periodic_holds_the_grid_where_delay_would_drift) {
    executor<8> ex;
    task_storage<512> st;
    std::vector<std::uint32_t> stamps;
    alloy::test::set_uptime_ms(1000);
    periodic p{std::chrono::milliseconds{100}};
    task t = metronome_task(st, p, stamps, 3);
    ex.spawn(t);
    ex.run_once();  // start; parks on the first next() (deadline 1100)

    // The executor is LATE to every wake by 30 ms — the exact situation where
    // `co_await delay(100ms)` ticks at 130 ms. The grid must not move.
    alloy::test::set_uptime_ms(1130);
    ex.run_once();  // woke for the 1100 edge (30 late)
    alloy::test::set_uptime_ms(1230);
    ex.run_once();  // the NEXT edge is 1200, not 1230+100
    alloy::test::set_uptime_ms(1330);
    ex.run_once();
    ALLOY_CHECK_EQ(stamps.size(), 3u);
    // Wakes happened late (that is life), but each was armed ON the grid:
    // 1100, 1200, 1300 — the phase never absorbed the lateness.
    ALLOY_CHECK_EQ(p.phase_ms(), 1400u);
}

ALLOY_TEST(async_periodic_starvation_catches_up_without_a_burst) {
    executor<8> ex;
    task_storage<512> st;
    std::vector<std::uint32_t> stamps;
    alloy::test::set_uptime_ms(2000);
    periodic p{std::chrono::milliseconds{100}};
    task t = metronome_task(st, p, stamps, 3);
    ex.spawn(t);
    ex.run_once();  // parks on deadline 2100

    // The task is starved for 3.7 periods (edges 2100/2200/2300/2400 owed).
    // What a starved periodic delivers is the PENDING wake (2100 — its timer
    // had already fired) plus ONE catch-up serve for the most recent missed
    // edge (2400, already past when next() realigns) — TWO serves, never the
    // four a naive replay would burst. Both land inside this superstep.
    alloy::test::set_uptime_ms(2470);
    ALLOY_CHECK_EQ(ex.run_once(), 1u);  // one RESUME runs both serves through
    ALLOY_CHECK_EQ(stamps.size(), 2u);  // the loop's immediate-ready next()
    // ...and the grid is intact: the task now waits for 2500, a multiple of
    // the interval from the construction epoch — no drift absorbed.
    ALLOY_CHECK_EQ(p.phase_ms(), 2600u);  // 2500 armed + one interval
    alloy::test::set_uptime_ms(2500);
    ex.run_once();
    ALLOY_CHECK_EQ(stamps.size(), 3u);
}

