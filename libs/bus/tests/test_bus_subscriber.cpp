// alloy::lib::bus::subscriber — queue semantics (FIFO, drop-newest counted)
// and both consumption shapes: try_next (no executor anywhere) and the
// co_await path driven exactly like the async suite drives event: publish
// plays the interrupt, run_once() resumes in thread context. The single-
// waiter contract is verified as a death test, same idiom as
// async_second_waiter_on_one_event_traps.

#include "bus.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>

#include "alloy/async/executor.hpp"
#include "alloy/async/task.hpp"
#include "alloy_test.hpp"

namespace {

struct reading {
    std::uint32_t v;
};

alloy::async::task consume(alloy::async::task_storage<256>&,
                           alloy::lib::bus::subscriber<reading, 4>& sub,
                           std::uint32_t* out, int n) {
    for (int i = 0; i < n; ++i) {
        const reading r = co_await sub.next();
        out[i] = r.v;
    }
}

}  // namespace

ALLOY_TEST(bus_subscriber_is_fifo) {
    alloy::lib::bus::subscriber<reading, 4> sub;
    (void)alloy::lib::bus::publish(reading{1});
    (void)alloy::lib::bus::publish(reading{2});
    (void)alloy::lib::bus::publish(reading{3});

    reading out{};
    ALLOY_CHECK(sub.try_next(out));
    ALLOY_CHECK_EQ(out.v, 1u);
    ALLOY_CHECK(sub.try_next(out));
    ALLOY_CHECK_EQ(out.v, 2u);
    ALLOY_CHECK(sub.try_next(out));
    ALLOY_CHECK_EQ(out.v, 3u);
    ALLOY_CHECK(!sub.try_next(out));
}

ALLOY_TEST(bus_subscriber_drops_newest_and_counts) {
    alloy::lib::bus::subscriber<reading, 2> sub;
    ALLOY_CHECK(alloy::lib::bus::publish(reading{1}));
    ALLOY_CHECK(alloy::lib::bus::publish(reading{2}));
    ALLOY_CHECK(!alloy::lib::bus::publish(reading{3}));  // full — dropped
    ALLOY_CHECK(!alloy::lib::bus::publish(reading{4}));  // still full
    ALLOY_CHECK_EQ(sub.missed(), 2u);

    // Drop-NEWEST: the queue kept the oldest two; 3 and 4 are gone.
    reading out{};
    ALLOY_CHECK(sub.try_next(out));
    ALLOY_CHECK_EQ(out.v, 1u);
    ALLOY_CHECK(sub.try_next(out));
    ALLOY_CHECK_EQ(out.v, 2u);
    ALLOY_CHECK(!sub.try_next(out));

    // Draining frees the queue; the counter is cumulative history, not state.
    ALLOY_CHECK(alloy::lib::bus::publish(reading{5}));
    ALLOY_CHECK_EQ(sub.missed(), 2u);
    ALLOY_CHECK(sub.try_next(out));
    ALLOY_CHECK_EQ(out.v, 5u);
}

ALLOY_TEST(bus_awaiting_task_is_woken_by_publish) {
    alloy::async::executor<8> ex;
    alloy::async::task_storage<256> st;
    alloy::lib::bus::subscriber<reading, 4> sub;
    std::uint32_t out[3] = {0, 0, 0};

    ex.spawn(consume(st, sub, out, 3));
    ex.run_once();  // task parks on an empty queue
    ALLOY_CHECK_EQ(out[0], 0u);

    (void)alloy::lib::bus::publish(reading{11});  // the "interrupt"
    ex.run_once();
    ALLOY_CHECK_EQ(out[0], 11u);  // resumed, consumed, parked again

    // Two publishes before one resume: first wakes, second lands in the
    // queue while the task is already scheduled. One superstep drains both
    // (the second co_await finds the queue non-empty and never parks).
    (void)alloy::lib::bus::publish(reading{12});
    (void)alloy::lib::bus::publish(reading{13});
    ex.run_once();
    ALLOY_CHECK_EQ(out[1], 12u);
    ALLOY_CHECK_EQ(out[2], 13u);
    ALLOY_CHECK(!st.in_use);  // consumed n=3 -> returned -> retired
}

ALLOY_TEST(bus_publish_before_await_resumes_immediately) {
    // The raced-ready case: the message is already queued when the task first
    // awaits. await_ready must see it and skip suspension entirely.
    alloy::async::executor<8> ex;
    alloy::async::task_storage<256> st;
    alloy::lib::bus::subscriber<reading, 4> sub;
    std::uint32_t out[1] = {0};

    (void)alloy::lib::bus::publish(reading{21});  // arrives first
    ex.spawn(consume(st, sub, out, 1));
    ex.run_once();
    ALLOY_CHECK_EQ(out[0], 21u);
    ALLOY_CHECK(!st.in_use);
}

ALLOY_TEST(bus_second_awaiting_task_traps) {
    // ONE awaiting task per subscriber. Two tasks parking on one subscriber
    // is a design error and must trap loudly, not silently starve one.
    // Death test: the child must NOT exit cleanly.
    const pid_t pid = fork();
    if (pid == 0) {
        alloy::async::executor<8> ex;
        alloy::async::task_storage<256> sa;
        alloy::async::task_storage<256> sb;
        alloy::lib::bus::subscriber<reading, 4> sub;
        std::uint32_t out[1] = {0};
        ex.spawn(consume(sa, sub, out, 1));
        ex.spawn(consume(sb, sub, out, 1));
        ex.run_once();  // A parks; B's park() finds the slot owned -> traps
        _exit(0);       // reached only if the guard FAILED to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    ALLOY_CHECK(!(WIFEXITED(status) && WEXITSTATUS(status) == 0));
}
