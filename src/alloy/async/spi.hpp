// alloy::async::spi_master — turns a SPI bus's transfer INTERRUPT into
//
//     co_await m.transfer(out, in);      // full duplex
//     co_await m.write(out);             // clock out, discard what comes back
//     co_await m.read(in);               // clock zeros, keep what comes back
//
// One suspend per TRANSFER, not per byte: the driver's ISR clocks every
// remaining byte and the task resumes in THREAD context when the last one
// lands. Compare handle::write(), which spins on TXE and RXNE for each byte.
// Nothing allocates — the awaiter lives on the coroutine frame.
//
// WHY THE START IS INSIDE THE AWAIT, AND WHY IN THAT ORDER
// -------------------------------------------------------
// The exchange is launched from await_suspend, AFTER the task is parked:
//
//     park(h)   ->   transfer_async()   ->   [ISR] wake()
//
// The waiter is registered before anything can complete, so the completion
// interrupt has nowhere to race to — there is no await_ready→await_suspend
// window for a wake to fall into, and therefore no readiness predicate to
// re-check under the mask (park() gets a constant-false one: the transfer
// provably has not started yet). It also makes the suspend UNCONDITIONAL, which
// matters for evidence as much as for correctness: a one-byte exchange that
// finishes before the first look would otherwise take an await_ready fast path,
// and a test that "passed" would have proven nothing about the interrupt.
//
// The cost is that a transfer cannot be kicked off and awaited later. Use
// handle::transfer_async() + busy() for that; this is the wait-for-it shape.
//
// SINGLE WAITER: at most one task may await one spi_master (a second traps in
// waiter_slot). A shared bus needs one owning task that fans out, not two
// awaiters — and the underlying driver traps on a concurrent transfer anyway.
//
// DO NOT MIX with the blocking xfer()/write()/transfer() on the same bus while
// an await is in flight: the ISR consumes the RXNE the blocking loop waits for.
//
// WHAT await_resume DOES NOT TELL YOU: nothing. It is `void` because the SPI
// driver reports no error — SPI has no ACK, and the ST backend deliberately
// leaves ERRIE unarmed (arming an error interrupt without implementing its
// clear sequence storms the NVIC). What came back is in the rx span; whether it
// is meaningful is the device protocol's business, not the bus's.

#pragma once

#include <coroutine>
#include <cstdint>
#include <span>

#include "alloy/async/waiter.hpp"

namespace alloy::async {

// Bus is any type exposing transfer_async(tx, rx, void(*)(void*), void*) — the
// alloy::spi::handle contract (present only on backends with an IRQ hook, so a
// port without one fails to compile here rather than silently blocking).
template <class Bus>
class spi_master {
    Bus& bus_;
    waiter_slot slot_;  // the reusable race-free park/wake mechanism

    // Transfer-complete interrupt context: the driver has already stored the
    // received bytes and cleared busy_, so there is no readiness source to
    // update here — just hand the parked task to the executor.
    static void on_done(void* ctx) { static_cast<spi_master*>(ctx)->slot_.wake(); }

public:
    explicit spi_master(Bus& b) : bus_(b) {}
    spi_master(const spi_master&) = delete;
    spi_master& operator=(const spi_master&) = delete;

    struct xfer_awaiter {
        spi_master& m;
        std::span<const std::uint8_t> tx;
        std::span<std::uint8_t> rx;

        // Nothing has been started, so nothing can be ready — see the header.
        bool await_ready() const noexcept { return false; }

        bool await_suspend(std::coroutine_handle<> h) noexcept {
            const bool parked = m.slot_.park(h, [] { return false; });
            m.bus_.transfer_async(tx, rx, &on_done, &m);  // now the ISR may fire
            return parked;
        }

        void await_resume() const noexcept {}
    };

    // Full duplex: tx and rx must be the same length (the driver traps if not).
    xfer_awaiter transfer(std::span<const std::uint8_t> tx,
                          std::span<std::uint8_t> rx) noexcept {
        return {*this, tx, rx};
    }

    xfer_awaiter write(std::span<const std::uint8_t> tx) noexcept { return {*this, tx, {}}; }

    xfer_awaiter read(std::span<std::uint8_t> rx) noexcept { return {*this, {}, rx}; }

    // Advisory: is a transfer still in flight? (Same source the driver polls.)
    [[nodiscard]] bool busy() const { return bus_.busy(); }
};

}  // namespace alloy::async
