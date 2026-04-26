// Host-side tests for the cooperative coroutine scheduler. Pure logic, no
// hardware. The tests inject a mock time source so deterministic ordering is
// possible without touching SysTick.
//
// Coverage: pool alloc/dealloc, spawn happy path, pool exhaustion, frame too
// large, FIFO within priority, priority preemption-at-yield, delay deadline
// honoured, yield_now requeues, cancellation interrupts a delay.

#include <array>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "runtime/tasks/channel.hpp"
#include "runtime/tasks/event.hpp"
#include "runtime/tasks/pool.hpp"
#include "runtime/tasks/scheduler.hpp"
#include "runtime/tasks/uart_channel.hpp"
#include "runtime/tasks/uart_tx_channel.hpp"

using namespace alloy;
using runtime::time::Duration;
using runtime::time::Instant;
using tasks::Cancelled;
using tasks::CancellationToken;
using tasks::Priority;
using tasks::Scheduler;
using tasks::SpawnError;
using tasks::Task;

namespace {

// --- mock time source -------------------------------------------------------
//
// A thread-local "now" advances under our control so delays resolve without
// real wall-clock waits.
std::uint64_t g_mock_micros = 0;

auto mock_now() -> Instant { return Instant::from_micros(g_mock_micros); }

void advance(Duration d) { g_mock_micros += d.as_micros(); }

void reset_clock() { g_mock_micros = 0; }

// --- pool tests --------------------------------------------------------------

TEST_CASE("FramePool reserves and frees slots", "[tasks][pool]") {
    tasks::FramePool<4, 128> pool;
    REQUIRE(pool.in_use() == 0);
    void* a = pool.allocate(64);
    void* b = pool.allocate(80);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(a != b);
    CHECK(pool.in_use() == 2);
    pool.deallocate(a);
    CHECK(pool.in_use() == 1);
    pool.deallocate(b);
    CHECK(pool.in_use() == 0);
}

TEST_CASE("FramePool rejects oversize requests without consuming a slot",
          "[tasks][pool]") {
    tasks::FramePool<2, 64> pool;
    CHECK(pool.allocate(128) == nullptr);
    CHECK(pool.last_oversize_request() == 128);
    CHECK(pool.in_use() == 0);
}

TEST_CASE("FramePool returns nullptr when full", "[tasks][pool]") {
    tasks::FramePool<2, 64> pool;
    REQUIRE(pool.allocate(32) != nullptr);
    REQUIRE(pool.allocate(32) != nullptr);
    CHECK(pool.allocate(32) == nullptr);
    CHECK(pool.in_use() == 2);
}

// --- scheduler/coroutine tests ----------------------------------------------

auto trivial_task(int* counter) -> Task {
    ++(*counter);
    co_return;
}

TEST_CASE("Scheduler runs a single trivial task to completion", "[tasks][sched]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    int counter = 0;
    auto r = sched.spawn([&] { return trivial_task(&counter); });
    REQUIRE(r.is_ok());
    sched.run();
    CHECK(counter == 1);
}

auto bumps_until_done(int* counter, int loops) -> Task {
    for (int i = 0; i < loops; ++i) {
        ++(*counter);
        co_await tasks::yield_now();
    }
}

TEST_CASE("yield_now interleaves tasks within a priority", "[tasks][sched]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    int a = 0;
    int b = 0;
    REQUIRE(sched.spawn([&] { return bumps_until_done(&a, 3); }).is_ok());
    REQUIRE(sched.spawn([&] { return bumps_until_done(&b, 3); }).is_ok());
    sched.run();
    CHECK(a == 3);
    CHECK(b == 3);
}

auto increments_after_delay(int* counter, Duration d) -> Task {
    static_cast<void>(co_await tasks::delay(d));
    ++(*counter);
}

TEST_CASE("delay honours the deadline against a mock clock", "[tasks][sched]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    int counter = 0;
    REQUIRE(sched.spawn([&] {
                 return increments_after_delay(&counter, Duration::from_millis(100));
             })
                .is_ok());

    // Tick once with the clock at 0: the task suspends at the delay.
    sched.tick();
    CHECK(counter == 0);

    // Advance 50 ms; tick should observe deadline not yet passed.
    advance(Duration::from_millis(50));
    sched.tick();
    CHECK(counter == 0);

    // Advance another 50 ms; the deadline is now reached, the task runs.
    advance(Duration::from_millis(50));
    sched.tick();  // moves Ready
    sched.tick();  // resumes
    CHECK(counter == 1);
}

TEST_CASE("Priority::High wins contention over Priority::Low", "[tasks][sched][priority]") {
    reset_clock();
    Scheduler<4, 256> sched;
    sched.set_time_source(mock_now);
    int order_index = 0;
    int high_seen_at = -1;
    int low_seen_at = -1;

    auto high = [&]() -> Task {
        high_seen_at = order_index++;
        co_return;
    };
    auto low = [&]() -> Task {
        low_seen_at = order_index++;
        co_return;
    };

    // Spawn low first; high after. In a strictly FIFO scheduler, low would
    // win. With priority, high goes first regardless of spawn order.
    REQUIRE(sched.spawn([&] { return low(); }, Priority::Low).is_ok());
    REQUIRE(sched.spawn([&] { return high(); }, Priority::High).is_ok());
    sched.run();

    CHECK(high_seen_at == 0);
    CHECK(low_seen_at == 1);
}

TEST_CASE("spawn returns FrameTooLarge when the coroutine frame exceeds the slot",
          "[tasks][sched]") {
    reset_clock();
    // A 32-byte slot cannot hold even the smallest coroutine frame.
    Scheduler<1, 32> sched;
    sched.set_time_source(mock_now);
    auto r = sched.spawn([&] { return trivial_task(nullptr); });
    REQUIRE(r.is_err());
    CHECK(r.unwrap_err() == SpawnError::FrameTooLarge);
}

TEST_CASE("spawn returns PoolFull when every slot is occupied", "[tasks][sched]") {
    reset_clock();
    Scheduler<1, 256> sched;
    sched.set_time_source(mock_now);
    auto first = sched.spawn([&] {
        return [&]() -> Task {
            co_await tasks::yield_now();
        }();
    });
    REQUIRE(first.is_ok());
    auto second = sched.spawn([&] {
        return [&]() -> Task { co_return; }();
    });
    REQUIRE(second.is_err());
    CHECK(second.unwrap_err() == SpawnError::PoolFull);
}

// --- Channel<T, N> tests -----------------------------------------------------

TEST_CASE("Channel try_push and try_pop round-trip a sequence of values",
          "[tasks][channel]") {
    tasks::Channel<int, 4> ch;
    REQUIRE(ch.empty());
    CHECK(ch.try_push(1));
    CHECK(ch.try_push(2));
    CHECK(ch.try_push(3));
    CHECK(ch.size() == 3);
    auto a = ch.try_pop();
    auto b = ch.try_pop();
    auto c = ch.try_pop();
    auto d = ch.try_pop();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    REQUIRE_FALSE(d.has_value());
    CHECK(*a == 1);
    CHECK(*b == 2);
    CHECK(*c == 3);
}

TEST_CASE("Channel drops values and bumps the counter when full",
          "[tasks][channel]") {
    tasks::Channel<int, 2> ch;  // capacity 2 -> usable slots = 1 (one always reserved)
    CHECK(ch.try_push(10));
    CHECK_FALSE(ch.try_push(11));    // full: head+1 == tail
    CHECK(ch.drops() == 1);
    auto v = ch.try_pop();
    REQUIRE(v.has_value());
    CHECK(*v == 10);
    CHECK(ch.try_push(12));
    CHECK(ch.drops() == 1);  // didn't bump on success
}

TEST_CASE("Channel wait() suspends until try_push signals", "[tasks][channel][sched]") {
    reset_clock();
    Scheduler<2, 384> sched;
    sched.set_time_source(mock_now);
    tasks::Channel<int, 8> ch;
    int sum = 0;

    auto consumer = [&]() -> Task {
        for (int i = 0; i < 3; ++i) {
            // drain ready values, then suspend for more
            while (auto v = ch.try_pop()) {
                sum += *v;
            }
            static_cast<void>(co_await ch.wait());
        }
        // final drain so any still-queued values are counted
        while (auto v = ch.try_pop()) {
            sum += *v;
        }
    };

    REQUIRE(sched.spawn([&] { return consumer(); }).is_ok());
    sched.tick();  // first iteration drains nothing, suspends

    // Three "ISR firings" with batches; consumer wakes once per signal,
    // drains everything ready, suspends again.
    REQUIRE(ch.try_push(1));
    REQUIRE(ch.try_push(2));
    sched.tick();  // wake
    sched.tick();  // resume; drains 1+2=3, suspends again

    REQUIRE(ch.try_push(10));
    sched.tick();
    sched.tick();

    REQUIRE(ch.try_push(100));
    sched.tick();
    sched.tick();  // resume + final drain happens on completion path

    CHECK(sum == 1 + 2 + 10 + 100);
}

// --- UartRxChannel tests -----------------------------------------------------

TEST_CASE("UartRxChannel feeds bytes from a fake ISR through the scheduler",
          "[tasks][uart][sched]") {
    reset_clock();
    Scheduler<2, 384> sched;
    sched.set_time_source(mock_now);
    tasks::UartRxChannel<8> uart_rx;
    std::uint32_t echoed = 0;
    std::uint32_t checksum = 0;

    auto echo_task = [&]() -> Task {
        for (int round = 0; round < 3; ++round) {
            while (auto byte = uart_rx.try_pop()) {
                ++echoed;
                checksum = (checksum + static_cast<std::uint32_t>(*byte)) & 0xFFu;
            }
            static_cast<void>(co_await uart_rx.wait());
        }
        // final drain so any still-queued bytes count
        while (auto byte = uart_rx.try_pop()) {
            ++echoed;
            checksum = (checksum + static_cast<std::uint32_t>(*byte)) & 0xFFu;
        }
    };

    REQUIRE(sched.spawn([&] { return echo_task(); }).is_ok());
    sched.tick();  // task hits await on empty channel and suspends

    // Three "ISR firings", each dropping one or more bytes into the ring.
    CHECK(uart_rx.feed_from_isr(std::uint8_t{0x10}));
    CHECK(uart_rx.feed_from_isr(std::uint8_t{0x20}));
    sched.tick();  // wakes the task
    sched.tick();  // task resumes, drains, suspends again

    CHECK(uart_rx.feed_from_isr(std::uint8_t{0x05}));
    sched.tick();
    sched.tick();

    CHECK(uart_rx.feed_from_isr(std::byte{0xFF}));
    sched.tick();
    sched.tick();  // resume + final-drain path

    CHECK(echoed == 4);
    CHECK(checksum == ((0x10 + 0x20 + 0x05 + 0xFF) & 0xFFu));
    CHECK(uart_rx.drops() == 0);
}

TEST_CASE("UartRxChannel reports drops when the ring is full",
          "[tasks][uart]") {
    tasks::UartRxChannel<2> rx;  // 2 -> usable slots = 1 (one always reserved)
    CHECK(rx.feed_from_isr(std::byte{0xAA}));
    CHECK_FALSE(rx.feed_from_isr(std::byte{0xBB}));
    CHECK_FALSE(rx.feed_from_isr(std::byte{0xCC}));
    CHECK(rx.drops() == 2);
    CHECK(rx.try_pop().has_value());
    CHECK(rx.feed_from_isr(std::byte{0xDD}));  // slot freed; succeeds
    CHECK(rx.drops() == 2);                    // unchanged on success
}

// --- UartTxChannel tests -----------------------------------------------------

TEST_CASE("UartTxChannel try_send queues bytes and pop_for_isr drains them",
          "[tasks][uart_tx]") {
    tasks::UartTxChannel<8> tx;
    REQUIRE(tx.empty());
    CHECK(tx.try_send(std::byte{0xAA}));
    CHECK(tx.try_send(std::byte{0xBB}));
    CHECK(tx.try_send(std::byte{0xCC}));
    CHECK(tx.size() == 3);

    auto a = tx.pop_for_isr();
    auto b = tx.pop_for_isr();
    auto c = tx.pop_for_isr();
    auto d = tx.pop_for_isr();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    REQUIRE_FALSE(d.has_value());
    CHECK(*a == std::byte{0xAA});
    CHECK(*b == std::byte{0xBB});
    CHECK(*c == std::byte{0xCC});
    CHECK(tx.empty());
}

TEST_CASE("UartTxChannel kicks the callback only on empty -> non-empty transition",
          "[tasks][uart_tx]") {
    tasks::UartTxChannel<4> tx;
    static int kicks;  // static so the lambda can capture-by-reference into the
                       // function pointer slot (KickFn is plain `void(*)()`)
    kicks = 0;
    tx.set_kick_callback([] { ++kicks; });

    // First push: queue was empty -> kick fires.
    CHECK(tx.try_send(std::byte{1}));
    CHECK(kicks == 1);

    // Subsequent pushes while non-empty: no kick (the ISR is supposed to be
    // running; kicking again would be redundant and could re-arm an
    // edge-triggered interrupt at the wrong moment).
    CHECK(tx.try_send(std::byte{2}));
    CHECK(tx.try_send(std::byte{3}));
    CHECK(kicks == 1);

    // ISR drains the queue.
    CHECK(tx.pop_for_isr().has_value());
    CHECK(tx.pop_for_isr().has_value());
    CHECK(tx.pop_for_isr().has_value());
    CHECK(tx.empty());

    // Next push transitions empty -> non-empty again -> kick fires.
    CHECK(tx.try_send(std::byte{4}));
    CHECK(kicks == 2);
}

TEST_CASE("UartTxChannel reports drops when the ring is full",
          "[tasks][uart_tx]") {
    tasks::UartTxChannel<2> tx;  // 2 -> 1 usable slot
    CHECK(tx.try_send(std::byte{0xAA}));
    CHECK_FALSE(tx.try_send(std::byte{0xBB}));
    CHECK_FALSE(tx.try_send(std::byte{0xCC}));
    CHECK(tx.drops() == 2);
    CHECK(tx.pop_for_isr().has_value());
    CHECK(tx.try_send(std::byte{0xDD}));
    CHECK(tx.drops() == 2);  // unchanged on success
}

TEST_CASE("UartTxChannel back-pressure: task awaits wait_space() on full ring",
          "[tasks][uart_tx][sched]") {
    reset_clock();
    Scheduler<2, 384> sched;
    sched.set_time_source(mock_now);
    tasks::UartTxChannel<2> tx;  // tiny ring forces the back-pressure path
    int sent = 0;

    auto sender = [&]() -> Task {
        const std::byte payload[] = {std::byte{0x10}, std::byte{0x20},
                                     std::byte{0x30}, std::byte{0x40}};
        for (auto b : payload) {
            while (!tx.try_send(b)) {
                static_cast<void>(co_await tx.wait_space());
            }
            ++sent;
        }
    };

    REQUIRE(sched.spawn([&] { return sender(); }).is_ok());

    // First tick: pushes 0x10 (slot 0). next try_send for 0x20 finds the
    // ring full (cap=2 -> 1 usable slot) and suspends on wait_space.
    sched.tick();
    CHECK(sent == 1);
    CHECK(tx.full());

    // ISR drains one byte, signals popped.
    auto a = tx.pop_for_isr();
    REQUIRE(a.has_value());
    CHECK(*a == std::byte{0x10});

    // Wake the task; it pushes 0x20 and again hits the back-pressure path.
    sched.tick();  // wake
    sched.tick();  // resume + push + suspend
    CHECK(sent == 2);

    // Drain + wake + resume for the remaining bytes.
    REQUIRE(tx.pop_for_isr().has_value());
    sched.tick();
    sched.tick();
    REQUIRE(tx.pop_for_isr().has_value());
    sched.tick();
    sched.tick();

    CHECK(sent == 4);
}

// --- until(predicate) tests --------------------------------------------------

TEST_CASE("until() resumes immediately when the predicate is already true",
          "[tasks][sched][until]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    int reached = 0;

    auto t = [&]() -> Task {
        bool always_true = true;
        static_cast<void>(co_await tasks::until([&] { return always_true; }));
        reached = 1;
    };

    REQUIRE(sched.spawn([&] { return t(); }).is_ok());
    sched.tick();
    CHECK(reached == 1);
}

TEST_CASE("until() suspends until the predicate flips true",
          "[tasks][sched][until]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    int counter = 0;
    bool ready = false;
    int reached = 0;

    auto t = [&]() -> Task {
        // Predicate captures `ready` by reference. The lambda lives in this
        // task's coroutine frame, so the reference stays valid across
        // suspensions. The scheduler polls the lambda every tick.
        static_cast<void>(co_await tasks::until([&] { return ready; }));
        reached = 1;
    };

    REQUIRE(sched.spawn([&] { return t(); }).is_ok());

    // First tick: task suspends; predicate false.
    sched.tick();
    ++counter;
    CHECK(reached == 0);

    // Several ticks while predicate stays false: task remains suspended,
    // counter advances on the test side just to prove time is moving.
    for (int i = 0; i < 5; ++i) {
        sched.tick();
        ++counter;
    }
    CHECK(reached == 0);

    // Flip the predicate; the next tick observes it and wakes the task.
    ready = true;
    sched.tick();  // wake (transition to Ready)
    sched.tick();  // resume; reached := 1
    CHECK(reached == 1);
    CHECK(counter == 6);  // sanity: ticks happened
}

TEST_CASE("Cancelling an until() wait returns Cancelled", "[tasks][sched][until][cancel]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    auto token = CancellationToken::make();
    bool got_cancel = false;
    bool never_true = false;

    auto t = [&]() -> Task {
        auto r = co_await tasks::until([&] { return never_true; });
        got_cancel = r.is_err();
    };

    REQUIRE(sched.spawn([&] { return t(); }, Priority::Normal, token).is_ok());
    sched.tick();  // suspends
    REQUIRE_FALSE(got_cancel);

    token.request();
    sched.tick();  // wake via cancellation
    sched.tick();  // resume; awaiter returns Cancelled
    CHECK(got_cancel);
}

// --- on(event) tests ---------------------------------------------------------

TEST_CASE("on(event) suspends until the event is signalled", "[tasks][sched][event]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    tasks::Event uart_byte_ready;
    int byte_count = 0;

    auto echo = [&]() -> Task {
        for (int i = 0; i < 3; ++i) {
            static_cast<void>(co_await tasks::on(uart_byte_ready));
            ++byte_count;
        }
    };

    REQUIRE(sched.spawn([&] { return echo(); }).is_ok());

    // Tick once: task hits the await and suspends; no event fired yet.
    sched.tick();
    CHECK(byte_count == 0);

    // Simulate three ISR firings, draining each before the next.
    for (int i = 0; i < 3; ++i) {
        uart_byte_ready.signal();
        sched.tick();  // wakes the task (state -> Ready)
        sched.tick();  // resumes it; bumps counter; awaits again or completes
    }

    CHECK(byte_count == 3);
}

TEST_CASE("on(event) consumes a pre-signalled event without suspending",
          "[tasks][sched][event]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    tasks::Event already_set;
    already_set.signal();
    int reached = 0;

    auto t = [&]() -> Task {
        // Event was set before await; await_ready returns true, no suspend,
        // body continues immediately on the same tick.
        static_cast<void>(co_await tasks::on(already_set));
        reached = 1;
    };

    REQUIRE(sched.spawn([&] { return t(); }).is_ok());
    sched.tick();
    CHECK(reached == 1);
    CHECK_FALSE(already_set.is_signaled());
}

TEST_CASE("Cancelling an on(event) wait returns Cancelled", "[tasks][sched][event][cancel]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    tasks::Event never_signalled;
    auto token = CancellationToken::make();
    bool got_cancel = false;

    auto t = [&]() -> Task {
        auto r = co_await tasks::on(never_signalled);
        got_cancel = r.is_err();
    };

    REQUIRE(sched.spawn([&] { return t(); }, Priority::Normal, token).is_ok());
    sched.tick();  // suspends on event
    REQUIRE_FALSE(got_cancel);

    token.request();
    sched.tick();  // wakes via cancellation
    sched.tick();  // resumes; awaiter returns Cancelled
    CHECK(got_cancel);
}

TEST_CASE("Cancelling a delay returns Cancelled to the awaiter", "[tasks][sched][cancel]") {
    reset_clock();
    Scheduler<2, 256> sched;
    sched.set_time_source(mock_now);
    bool got_cancel = false;

    auto token = CancellationToken::make();

    auto cancellable = [&]() -> Task {
        auto r = co_await tasks::delay(Duration::from_millis(1000));
        got_cancel = r.is_err();
    };

    REQUIRE(sched.spawn([&] { return cancellable(); }, Priority::Normal, token).is_ok());

    // Tick once: task hits the delay and suspends.
    sched.tick();
    REQUIRE_FALSE(got_cancel);

    // Cancel the token; the next tick should resume the task and the awaiter
    // returns Cancelled.
    token.request();
    sched.tick();  // wakes the task
    sched.tick();  // resumes it; awaiter returns Cancelled
    CHECK(got_cancel);
}

}  // namespace
