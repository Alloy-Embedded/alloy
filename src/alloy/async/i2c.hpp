// alloy::async::i2c_master — turns an I2C bus's transfer INTERRUPT into
//
//     bool ok = co_await m.write(addr, out);
//     ok      = co_await m.read(addr, in);
//
// One suspend per TRANSFER, not per byte: the driver's ISR moves every byte and
// the task resumes in THREAD context when the STOP lands. Compare
// handle::write(), which spins once per byte plus once for the STOP — roughly
// 90 µs of burnt CPU per byte at 100 kHz. Nothing allocates; the awaiter lives
// on the coroutine frame.
//
// WHY THE START IS INSIDE THE AWAIT, AND WHY IN THAT ORDER
// -------------------------------------------------------
// The transfer is launched from await_suspend, AFTER the task is parked:
//
//     park(h)   ->   write_async()/read_async()   ->   [ISR] wake()
//
// The waiter is registered before anything can complete, so the completion
// interrupt has nowhere to race to — there is no await_ready→await_suspend
// window for a wake to fall into, and therefore no readiness predicate to
// re-check under the mask (park() gets a constant-false one: the transfer
// provably has not started yet). It also makes the suspend UNCONDITIONAL, which
// matters for evidence as much as for correctness: a transfer that finished
// before the first look would otherwise take an await_ready fast path, and a
// test that "passed" would have proven nothing about the interrupt.
//
// The cost is that a transfer cannot be kicked off and awaited later. Use
// handle::write_async() + busy() for that; this is the wait-for-it shape.
//
// SINGLE WAITER: at most one task may await one i2c_master (a second traps in
// waiter_slot). A shared bus needs one owning task that fans out, not two
// awaiters — and the underlying driver traps on a concurrent transfer anyway.
//
// DO NOT MIX with the blocking write()/read() on the same bus while an await is
// in flight: the ISR consumes the TXIS/RXNE/STOPF flags the blocking loop waits
// for, and both then give wrong answers.
//
// NO ASYNC write_read: the driver issues AUTOEND transfers, one direction per
// START, so the repeated-start combined transfer has no interrupt-driven form
// here. Two awaits (write then read) send a STOP in between — correct for most
// devices, wrong for the ones that require a repeated start. Use the blocking
// write_read() for those.

#pragma once

#include <coroutine>
#include <cstdint>
#include <span>

#include "alloy/async/waiter.hpp"

namespace alloy::async {

// Bus is any type exposing write_async/read_async(addr, span, void(*)(void*),
// void*) and transfer_ok() — the alloy::i2c::handle contract (present only on
// backends with an IRQ hook, so a port without one fails to compile here rather
// than silently blocking).
template <class Bus>
class i2c_master {
    Bus& bus_;
    waiter_slot slot_;  // the reusable race-free park/wake mechanism

    // STOP interrupt context: the driver has already latched the result and
    // cleared busy_, so there is no readiness source to update here — just hand
    // the parked task to the executor.
    static void on_done(void* ctx) { static_cast<i2c_master*>(ctx)->slot_.wake(); }

public:
    explicit i2c_master(Bus& b) : bus_(b) {}
    i2c_master(const i2c_master&) = delete;
    i2c_master& operator=(const i2c_master&) = delete;

    // Exactly one of tx/rx is non-empty; rx.empty() selects the write.
    struct op_awaiter {
        i2c_master& m;
        std::uint8_t addr;
        std::span<const std::uint8_t> tx;
        std::span<std::uint8_t> rx;

        // Nothing has been started, so nothing can be ready — see the header.
        bool await_ready() const noexcept { return false; }

        bool await_suspend(std::coroutine_handle<> h) noexcept {
            const bool parked = m.slot_.park(h, [] { return false; });
            if (rx.empty()) {  // now the ISR may fire
                m.bus_.write_async(addr, tx, &on_done, &m);
            } else {
                m.bus_.read_async(addr, rx, &on_done, &m);
            }
            return parked;
        }

        // False on NACK, bus error, or an abandoned transfer — the same bool
        // contract the blocking write()/read() return.
        [[nodiscard]] bool await_resume() const noexcept { return m.bus_.transfer_ok(); }
    };

    op_awaiter write(std::uint8_t addr, std::span<const std::uint8_t> data) noexcept {
        return {*this, addr, data, {}};
    }

    op_awaiter read(std::uint8_t addr, std::span<std::uint8_t> data) noexcept {
        return {*this, addr, {}, data};
    }

    // Advisory: is a transfer still in flight? (Same source the driver polls.)
    [[nodiscard]] bool busy() const { return bus_.busy(); }
};

}  // namespace alloy::async
