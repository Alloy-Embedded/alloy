// alloy::lib::bus::subscriber<T, Depth> — a private queue on a topic, with
// the two consumption shapes the framework supports: try_next() for a
// superloop (works with no executor at all — waiter_slot::wake() is a no-op
// when nobody is parked), and co_await sub.next() for a coroutine task.
//
// Each subscriber owns its own ring_buffer, so backpressure is ISOLATED: a
// slow consumer drops ITS OWN messages, never a sibling's. The overflow
// policy is drop-newest, counted — the same answer async::uart_reader gives
// ("like a HW overrun") and the only one the sanctioned SPSC ring permits:
// drop-oldest would need a producer-side pop, and ring_buffer's lock-free
// contract is one producer, one consumer, each owning one index. If a real
// use case ever needs drop-oldest, that is an RFC on ring_buffer, not a
// policy flag here.
//
// Lifetime rules (the house rules for long-lived awaitable owners, same as
// uart_reader/dma_waiter):
//   - Construct the subscriber OUTSIDE the coroutine, in storage that
//     outlives every task that awaits it. The destructor unlinks from the
//     topic, so a scoped subscriber is safe — but a task still parked on it
//     at destruction is not (the slot would hold a handle nobody wakes).
//   - ONE awaiting task. A second task co_awaiting the same subscriber traps
//     in waiter_slot::park — a shared feed wants one owning task that fans
//     out, or two subscribers.
//
// missed() is a cumulative drop counter in the rx_stream::missed() mold: the
// overrun evidence, read as an advisory (aligned word read; exact under the
// publisher's mask, racy-by-one from thread context — fine for a witness).

#pragma once

#include <coroutine>
#include <cstddef>
#include <cstdint>

#include "alloy/async/waiter.hpp"
#include "alloy/util/ring_buffer.hpp"
#include "bus/topic.hpp"

namespace alloy::lib::bus {

template <class T, std::size_t Depth = 4>
class subscriber {
    static_assert(Depth >= 1, "a subscriber with no queue cannot receive");

public:
    subscriber() noexcept {
        node_.deliver = &deliver_thunk;
        node_.owner = this;
        detail::topic<T>::link(node_);
    }
    ~subscriber() { detail::topic<T>::unlink(node_); }
    subscriber(const subscriber&) = delete;
    subscriber& operator=(const subscriber&) = delete;

    // Superloop / poll shape. Never parks, never touches the executor.
    [[nodiscard]] bool try_next(T& out) noexcept { return q_.pop(out); }

    // Coroutine shape: co_await sub.next() suspends until a message arrives,
    // then pops it. The awaiter is the mandated plain-struct composition of
    // waiter_slot with this queue as the readiness source: park() re-checks
    // under the mask, so a publish racing into the await_ready→await_suspend
    // window is never lost.
    [[nodiscard]] auto next() noexcept {
        struct awaiter {
            subscriber& s;
            [[nodiscard]] bool await_ready() const noexcept { return !s.q_.empty(); }
            bool await_suspend(std::coroutine_handle<> h) noexcept {
                return s.slot_.park(h, [this] { return !s.q_.empty(); });
            }
            [[nodiscard]] T await_resume() noexcept {
                T v{};
                if (!s.q_.pop(v)) {
                    __builtin_trap();  // resumed with an empty queue: a second
                                       // consumer exists — contract violation
                }
                return v;
            }
        };
        return awaiter{*this};
    }

    // Cumulative count of messages dropped because the queue was full.
    [[nodiscard]] std::uint32_t missed() const noexcept { return missed_; }

private:
    // Runs under publish()'s irq mask, from whatever context published.
    static bool deliver_thunk(void* owner, const void* msg) noexcept {
        auto& self = *static_cast<subscriber*>(owner);
        if (!self.q_.push(*static_cast<const T*>(msg))) {
            ++self.missed_;
            return false;  // drop-newest: the queue keeps what it had
        }
        self.slot_.wake();  // no-op unless a task is parked; schedule() is
                            // idempotent for an already-queued task
        return true;
    }

    detail::node node_{};
    ring_buffer<T, Depth> q_;
    async::waiter_slot slot_;
    std::uint32_t missed_ = 0;
};

}  // namespace alloy::lib::bus
