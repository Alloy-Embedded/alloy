// alloy::async::event — a one-shot wake object a driver holds. `co_await ev`
// parks the current task until `ev.set()` (called from the driver's ISR) wakes
// it. The race-free park/wake is delegated to waiter_slot, so this is just the
// edge-latch readiness source on top of it — no re-derivation of the wakeup
// protocol. Single-owner (one task per event) is enforced by waiter_slot.

#pragma once

#include <coroutine>

#include "alloy/arch/irq.hpp"
#include "alloy/async/waiter.hpp"

namespace alloy::async {

class event {
    waiter_slot slot_;
    // Latched by set() (ISR) and read on the unmasked await_ready fast path, so
    // volatile forces the reload there; all other touches are under a mask.
    volatile bool signalled_ = false;

public:
    // From ISR context: latch the signal and wake the parked waiter, if any.
    void set() {
        const arch::irq_state s = arch::irq_save();
        signalled_ = true;
        arch::irq_restore(s);
        slot_.wake();
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

        bool await_suspend(std::coroutine_handle<> h) noexcept {
            // Re-check the latch under the mask via waiter_slot — closes the race.
            return e.slot_.park(h, [&] { return static_cast<bool>(e.signalled_); });
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
