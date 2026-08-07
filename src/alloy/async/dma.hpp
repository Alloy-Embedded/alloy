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
// WHAT await_resume DOES NOT TELL YOU: nothing. It is `void`, and that is not
// an oversight — the ST DMA ISR clears the channel's flags (it must, or the
// level-triggered line re-fires forever) BEFORE it invokes the callback, so by
// the time the task resumes, error() reads false whether or not the transfer
// failed. The existing on_complete() callback API has the same gap. Reporting a
// DMA error through the async path needs an error latch in the driver
// alongside the completion latch; this header does not add one, and returning a
// bool that is always true would be a lie rather than an API.

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

}  // namespace alloy::async
