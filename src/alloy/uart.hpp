// User-facing UART: typed pin binding with compile-time route checking,
// move-only handle satisfying ByteStream.
//
//   using Dbg = alloy::uart::bind<dev::usart2_t,
//                                 alloy::uart::tx<dev::pa2_t>,
//                                 alloy::uart::rx<dev::pa3_t>, Clock>;
//   auto u = Dbg::open({.baud = 115'200});
//
// A pin with no route to the peripheral fails the static_assert below with
// the pin and signal named in the message.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/uart/uart_impl.hpp"

namespace alloy::uart {

struct config {
    std::uint32_t baud = 115'200;
};

// Full line shape (parity/stop/inversion/hardware-DE) for drivers that can
// program it — see hal::serial_config. Offered through bind::reconfigure(),
// capability-gated: a board whose driver only speaks baud doesn't grow a
// method that lies.
using serial_config = hal::serial_config;

template <class Pin>
struct tx {
    using pin = Pin;
};
template <class Pin>
struct rx {
    using pin = Pin;
};

// Move-only handle: opening twice is a runtime trap (C++ cannot make a
// cross-TU double-open a compile error — see NORTH_STAR guard #7).
template <class Inst>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    void write(std::uint8_t byte) const { hal::uart_impl<Inst>::write(byte); }
    void write(const char* zstr) const {
        while (*zstr != '\0') {
            hal::uart_impl<Inst>::write(static_cast<std::uint8_t>(*zstr++));
        }
    }
    [[nodiscard]] bool read(std::uint8_t& byte) const { return hal::uart_impl<Inst>::read(byte); }
    void flush() const { hal::uart_impl<Inst>::flush(); }

    // Register a function to run in the RX interrupt: the driver's ISR
    // drains the hardware and calls fn(ctx, byte) per byte received. Only
    // exists where the backing driver implements interrupts (Cortex-M v1).
    void on_receive(void (*fn)(void*, std::uint8_t), void* ctx = nullptr) const
        requires requires { hal::uart_impl<Inst>::enable_rx_irq(fn, ctx); }
    {
        hal::uart_impl<Inst>::enable_rx_irq(fn, ctx);
    }
    void detach_receive() const
        requires requires { hal::uart_impl<Inst>::disable_rx_irq(); }
    {
        hal::uart_impl<Inst>::disable_rx_irq();
    }

    // Blocking TX of a buffer via a claimed DMA channel: returns once the
    // DMA finished AND the transmitter drained (honest completion). Only
    // exists where the driver has TX-DMA hooks and the chip data carries
    // the request ID.
    template <class Chan>
    [[nodiscard]] bool write_dma(const Chan& dma, std::span<const std::uint8_t> data) const
        requires requires {
            hal::uart_impl<Inst>::dma_tx_begin();
            Inst::dmareq_tx;
        }
    {
        write_dma_begin(dma, data);
        const bool ok = dma.wait();
        write_dma_end(dma);
        return ok;
    }

    // The two halves of write_dma(), split so the wait in the middle can be a
    // `co_await` instead of a spin (alloy::async::dma_waiter). Call them in
    // order and wait for the channel between them:
    //
    //     co_await w.run([&] { uart.write_dma_begin(chan, msg); });
    //     uart.write_dma_end(chan);
    //
    // write_dma() itself is now written in terms of these, so there is one
    // sequence, not two that can drift.
    //
    // HONEST NOTE on write_dma_end(): it still SPINS, on the transmitter's
    // TC flag — the DMA finishing means the last byte reached TDR, not that it
    // reached the wire. That is one character time (~87 µs at 115200 baud) and
    // this change does not remove it; TC has no interrupt path in the driver.
    template <class Chan>
    void write_dma_begin(const Chan& dma, std::span<const std::uint8_t> data) const
        requires requires {
            hal::uart_impl<Inst>::dma_tx_begin();
            Inst::dmareq_tx;
        }
    {
        hal::uart_impl<Inst>::dma_tx_begin();
        dma.start_m2p_u8(data, hal::uart_impl<Inst>::tdr_addr(), Inst::dmareq_tx);
    }

    template <class Chan>
    void write_dma_end(const Chan& dma) const
        requires requires { hal::uart_impl<Inst>::dma_tx_end(); }
    {
        dma.stop();
        hal::uart_impl<Inst>::dma_tx_end();
    }

private:
    template <class, class, class, class>
    friend struct bind;
    template <class>
    friend struct rom_bind;
    handle() = default;
};

// For UARTs fully configured by a boot ROM/bootloader (classic ESP32 UART0):
// no pin routing, no clock math — open() defers entirely to the driver.
template <class Inst>
struct rom_bind {
    static handle<Inst> open(config c = {}) {
        hal::uart_impl<Inst>::enable(0u, c.baud);
        return handle<Inst>{};
    }
};

template <class Inst, class Tx, class Rx, class Clock>
struct bind {
    using tx_pin = typename Tx::pin;
    using rx_pin = typename Rx::pin;

    static_assert(routes::routable<tx_pin, Inst, signal::tx>,
                  "TX pin has no route to this UART on the selected chip "
                  "(check the chip's route table in alloy-devices)");
    static_assert(routes::routable<rx_pin, Inst, signal::rx>,
                  "RX pin has no route to this UART on the selected chip "
                  "(check the chip's route table in alloy-devices)");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    // Route payload -> mux value per kind: af_fixed carries an AF number,
    // funcsel a FUNCSEL value; both reach the pin driver's make_af().
    template <class Route>
    static constexpr std::uint8_t mux_value() {
        static_assert(Route::k == routes::kind::af_fixed ||
                          Route::k == routes::kind::funcsel,
                      "route kind not implemented yet (full_matrix/psel pending)");
        if constexpr (Route::k == routes::kind::af_fixed) {
            return Route::af;
        } else {
            return Route::funcsel;
        }
    }

    static handle<Inst> open(config c) {
        using tx_route = routes::route<tx_pin, Inst, signal::tx>;
        using rx_route = routes::route<rx_pin, Inst, signal::rx>;

        if (detail_opened) {
            __builtin_trap();  // double-open: honest runtime guard
        }
        detail_opened = true;

        hal::pin_impl<tx_pin>::make_af(mux_value<tx_route>());
        hal::pin_impl<rx_pin>::make_af(mux_value<rx_route>());
        hal::uart_impl<Inst>::enable(kernel_hz(), c.baud);
        return handle<Inst>{};
    }

    // Reprogram the full line shape on a RUNNING port — the RS-485/multidrop
    // path: a protocol write changes baud/parity, the ACK goes out at the old
    // settings, then this applies the new ones (UE-low rewrite + TEACK/REACK
    // waits live in the driver). Only exists where the driver implements
    // configure().
    static void reconfigure(serial_config c)
        requires requires { hal::uart_impl<Inst>::configure(0u, c); }
    {
        hal::uart_impl<Inst>::configure(kernel_hz(), c);
    }

    // Compile-time-checked open: the requested baud is REJECTED AT COMPILE TIME
    // if the driver's own divisor cannot reach it within TolPermille (parts per
    // thousand of the target; default 20 = 2%) on this board's kernel clock.
    // The achieved rate is computed by the SAME formula the driver programs, so
    // the check can never disagree with the hardware, and the failing
    // rate_check<requested, achieved, tol> names the numbers. Only offered where
    // the backing driver computes baud — ROM/bootloader-baud UARTs don't.
    //
    //   auto u = Dbg::open_checked<115'200_baud>();        // 2% default
    //   auto u = Dbg::open_checked<3'000'000_baud, 5>();   // tighten to 0.5%
    template <alloy::frequency Baud, std::uint32_t TolPermille = 20>
    static handle<Inst> open_checked()
        requires requires { hal::uart_impl<Inst>::achieved_baud(kernel_hz(), Baud.hz()); }
    {
        constexpr alloy::frequency achieved =
            hal::uart_impl<Inst>::achieved_baud(kernel_hz(), Baud.hz());
        (void)alloy::rate_check<Baud.hz(), achieved.hz(), TolPermille>{};
        return open(config{.baud = Baud.hz()});
    }

private:
    inline static bool detail_opened = false;
};

}  // namespace alloy::uart
