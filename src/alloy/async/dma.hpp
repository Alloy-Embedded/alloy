// alloy::async::dma_waiter — turns a DMA channel's completion INTERRUPT into
//
//     co_await w.run([&] { ch.start_m2p_u8(msg, tdr, req); });
//
// The task suspends while the engine moves the bytes and resumes in THREAD
// context when the channel's ISR fires. Nothing allocates; the awaiter and the
// start callable live on the coroutine frame.
//
// WHY THE START IS INSIDE THE AWAIT, AND WHY IN THAT ORDER
// -------------------------------------------------------
// The transfer is launched from await_suspend, AFTER the task is parked:
//
//     park(h)   ->   start()   ->   [ISR] wake()
//
// The waiter is registered before anything can complete, so the completion
// interrupt has nowhere to race to — there is no await_ready→await_suspend
// window for a wake to fall into, and therefore no readiness predicate to
// re-check under the mask (park() gets a constant-false one: the operation
// provably has not started yet). It also makes the suspend UNCONDITIONAL, which
// matters for evidence as much as for correctness: a peripheral that completes
// before the first look would otherwise take an await_ready fast path, and a
// test that "passed" would have proven nothing about the interrupt at all.
//
// The cost is that the start cannot be hoisted out of the await — you cannot
// kick a transfer off, do other work, and await it later. Poll ch.done() for
// that; this is the wait-for-it shape.
//
// ORDERING RULE INHERITED FROM THE DRIVER: the channel's config register may
// only be written while the channel is disabled, so the ST backend folds TCIE
// into CCR at setup() based on whether a callback is registered. The waiter
// registers in its CONSTRUCTOR for exactly that reason — construct it once,
// before the first transfer, and keep it alive (a global or a main() local that
// outlives the tasks), never inside the coroutine.
//
// SINGLE WAITER: at most one task may await one dma_waiter; a second traps in
// waiter_slot. One channel, one owning task.
//
// WHAT await_resume DOES NOT TELL YOU: nothing. It is `void` — but the reason
// has changed, and the gap it used to paper over is closed. This note used to
// say that a resumed task could not learn whether the transfer failed, because
// the ST DMA ISR clears the channel's flags (it must, or the level-triggered
// line re-fires forever) BEFORE invoking the callback, leaving error() reading
// false either way. Both ST engines now LATCH the error they consumed, exactly
// as they latch completion (st_dma_v{1,2}_body.hpp; witnessed by
// test_st_dma_v{1,2}_latch.cpp), so after the task resumes `ch.error()` is
// truthful and so is `ch.wait()`. Ask the channel.
//
// await_resume stays void because widening it is an API decision for the
// awaiter, not a consequence of the driver fix: the failure is now
// OBSERVABLE, which is what was missing.

#pragma once

#include <coroutine>

#include "alloy/async/waiter.hpp"

namespace alloy::async {

// Chan is any type exposing on_complete(void(*)(void*), void*) and done() —
// the alloy::dma::channel contract.
template <class Chan>
class dma_waiter {
    Chan& ch_;
    waiter_slot slot_;  // the reusable race-free park/wake mechanism

    // DMA interrupt context: the channel state IS the readiness source (the
    // driver already latched completion for us), so there is nothing to update
    // here — just hand the parked task to the executor.
    static void on_done(void* ctx) { static_cast<dma_waiter*>(ctx)->slot_.wake(); }

public:
    explicit dma_waiter(Chan& c) : ch_(c) { ch_.on_complete(&on_done, this); }
    dma_waiter(const dma_waiter&) = delete;
    dma_waiter& operator=(const dma_waiter&) = delete;

    // Start is a caller-supplied callable because a DMA start is typed per
    // direction (start_m2p_u8 / start_p2m_u16 / …) and takes the peripheral
    // address and request line with it. A capturing lambda is fine and does not
    // allocate: it lives on the coroutine frame for the duration of the
    // co_await full-expression, and is only ever called from await_suspend.
    template <class Start>
    struct run_awaiter {
        dma_waiter& w;
        Start& start;

        // Nothing has been started, so nothing can be ready — see the header.
        bool await_ready() const noexcept { return false; }

        bool await_suspend(std::coroutine_handle<> h) noexcept {
            const bool parked = w.slot_.park(h, [] { return false; });
            start();  // only now can the completion interrupt fire
            return parked;
        }

        void await_resume() const noexcept {}
    };

    template <class Start>
    run_awaiter<Start> run(Start&& start) noexcept {
        return {*this, start};
    }

    // Advisory: has the channel finished? (Same source wait()/done() poll.)
    [[nodiscard]] bool done() const { return ch_.done(); }
};

// alloy::async::ring_waiter — turns a dma::ring's half/full boundary events
// into `std::span<const T> half = co_await w.take();`. The ring keeps its own
// two-slot latch and missed() accounting; this class only parks the task on
// "a stable half is pending" and lets the ring's blocking take() do the
// hand-over — which cannot spin after a resume, because the readiness the
// task woke on IS take()'s spin condition.
//
// Unlike dma_waiter there is no start-inside-the-await: the ring was started
// at construction and runs continuously, so the awaitable takes the
// uart_reader shape — ready fast path when a half is already pending,
// park-with-recheck-under-mask otherwise (waiter_slot closes the
// await_ready→await_suspend window).
//
// Construct it once, after the ring, and keep it alive alongside the ring
// (same lifetime rule as dma_waiter: the ring's ISR path holds the hook).
// Single waiter, like every waiter_slot user. A consumer that co_awaits
// slower than halves go stable does not deadlock — it wakes on the next
// boundary, take() resynchronizes, and the skipped halves are in missed().
//
// Ring is any type exposing pending()/take()/on_boundary(void(*)(void*),
// void*) — the alloy::dma::ring contract.
template <class Ring>
class ring_waiter {
    Ring& ring_;
    waiter_slot slot_;

    // Boundary ISR context: the ring already updated its latch (readiness
    // source); just hand the parked task to the executor.
    static void on_boundary(void* ctx) {
        static_cast<ring_waiter*>(ctx)->slot_.wake();
    }

public:
    explicit ring_waiter(Ring& r) : ring_(r) {
        ring_.on_boundary(&on_boundary, this);
    }
    ring_waiter(const ring_waiter&) = delete;
    ring_waiter& operator=(const ring_waiter&) = delete;

    struct take_awaiter {
        ring_waiter& w;

        bool await_ready() const noexcept { return w.ring_.pending(); }

        bool await_suspend(std::coroutine_handle<> h) noexcept {
            return w.slot_.park(h, [&] { return w.ring_.pending(); });
        }

        auto await_resume() const noexcept { return w.ring_.take(); }
    };

    // The next hardware-stable half (most recent, if the task fell behind).
    take_awaiter take() noexcept { return {*this}; }
};

}  // namespace alloy::async
