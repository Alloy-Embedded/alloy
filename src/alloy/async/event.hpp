// alloy::async::event — a one-shot wake object a driver holds. `co_await ev`
// parks the current task until `ev.set()` (called from the driver's ISR) wakes
// it. The register-then-recheck-under-mask sequence that closes the lost-wakeup
// race is baked in HERE, once, so every driver awaiter that builds on event is
// race-free by construction and no driver author re-derives it.
//
// Single-owner: at most one task may await one event at a time; a second
// concurrent waiter traps (a shared peripheral needs a byte queue, not two
// waiters on one latch — see async uart).

#pragma once

#include <coroutine>

#include "alloy/arch/irq.hpp"
#include "alloy/async/executor.hpp"

namespace alloy::async {

class event {
    // Touched by both ISR (set) and thread (await). waiter_ is only ever accessed
    // under irq_save, so the mask's "memory" clobber orders it. signalled_ is also
    // read on the unmasked await_ready fast path, so it is volatile to force a
    // reload there.
    std::coroutine_handle<> waiter_{};
    volatile bool signalled_ = false;

public:
    // From ISR context: latch the signal and wake the parked waiter, if any. The
    // resume itself happens later in thread context (schedule() only enqueues).
    void set() {
        const arch::irq_state s = arch::irq_save();
        signalled_ = true;
        auto w = waiter_;
        waiter_ = {};
        arch::irq_restore(s);
        if (w) {
            executor_core::the().schedule(w);
        }
    }

    // Consume a latched signal without suspending. True if one was pending.
    bool poll() {
        const arch::irq_state s = arch::irq_save();
        const bool r = signalled_;
        signalled_ = false;
        arch::irq_restore(s);
        return r;
    }

    struct awaiter {
        event& e;

        bool await_ready() const noexcept { return e.signalled_; }  // fast path

        // Returns false to resume immediately when the signal raced in between
        // await_ready and here — this is the lost-wakeup fix.
        bool await_suspend(std::coroutine_handle<> h) noexcept {
            const arch::irq_state s = arch::irq_save();
            if (e.signalled_) {
                arch::irq_restore(s);
                return false;  // signalled in the window — do not park
            }
            if (e.waiter_) {
                arch::irq_restore(s);
                __builtin_trap();  // a second waiter on one event — not allowed
            }
            e.waiter_ = h;
            arch::irq_restore(s);
            return true;
        }

        void await_resume() noexcept {
            const arch::irq_state s = arch::irq_save();
            e.signalled_ = false;
            arch::irq_restore(s);
        }
    };

    awaiter operator co_await() noexcept { return {*this}; }
};

}  // namespace alloy::async
