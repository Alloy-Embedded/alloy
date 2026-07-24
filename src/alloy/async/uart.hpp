// alloy::async::uart_reader — turns a UART handle's RX interrupt into
// `std::uint8_t b = co_await reader.read();`. The RX ISR pushes bytes into a
// lock-free SPSC ring (util/ring_buffer) and wakes the single task parked on
// read(); the resume runs in thread context. A ring (not a single byte) is used
// so a burst that arrives while the task is busy is buffered, not lost — the fix
// for the "two bytes, one slot" hazard a plain latch has.
//
// Single reader: at most one task may await one uart_reader (a second traps).
// Sharing a UART means one owning task that fans bytes out, not two readers.
//
//   alloy::async::uart_reader rx{uart};   // uart = board::debug_uart::open(...)
//   alloy::task echo(alloy::async::task_storage<256>&, decltype(rx)& r, Uart& u) {
//       for (;;) { u.write(co_await r.read()); }
//   }

#pragma once

#include <coroutine>
#include <cstddef>
#include <cstdint>

#include "alloy/async/waiter.hpp"
#include "alloy/util/ring_buffer.hpp"

namespace alloy::async {

// Uart is any type exposing on_receive(void(*)(void*,uint8_t), void*) — the
// alloy::uart::handle contract. RxCap bounds the buffered burst.
template <class Uart, std::size_t RxCap = 64>
class uart_reader {
    Uart& uart_;
    ring_buffer<std::uint8_t, RxCap> rx_{};
    waiter_slot slot_;  // the reusable race-free park/wake mechanism

    // RX interrupt context: buffer the byte, then wake the parked reader (if any)
    // through the shared waiter_slot — no re-derived wakeup protocol here.
    static void on_byte(void* ctx, std::uint8_t b) {
        auto* self = static_cast<uart_reader*>(ctx);
        (void)self->rx_.push(b);  // full ring drops the byte, like a HW overrun
        self->slot_.wake();
    }

public:
    explicit uart_reader(Uart& u) : uart_(u) { uart_.on_receive(&on_byte, this); }
    uart_reader(const uart_reader&) = delete;
    uart_reader& operator=(const uart_reader&) = delete;

    struct read_awaiter {
        uart_reader& r;

        bool await_ready() const noexcept { return !r.rx_.empty(); }

        // Park on the ring's non-empty readiness; waiter_slot rechecks it under
        // the mask, so a byte arriving in the await_ready→here window is not lost.
        bool await_suspend(std::coroutine_handle<> h) noexcept {
            return r.slot_.park(h, [&] { return !r.rx_.empty(); });
        }

        std::uint8_t await_resume() const noexcept {
            std::uint8_t b{};
            (void)r.rx_.pop(b);
            return b;
        }
    };

    read_awaiter read() noexcept { return {*this}; }

    // How many buffered bytes are waiting (consumer-side snapshot).
    [[nodiscard]] std::size_t available() const { return rx_.size(); }
};

}  // namespace alloy::async
