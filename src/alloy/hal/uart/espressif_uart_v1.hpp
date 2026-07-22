// UART driver for the classic ESP32 in ROM-configured mode.
//
// The boot ROM + 2nd-stage bootloader leave UART0 at 115200 8N1 with pins
// routed via IO_MUX — hardware-validated in the old ecosystem with exactly
// this FIFO/STATUS-only access pattern. enable() therefore programs
// nothing; full CLKDIV/CONF0 bring-up arrives with matrix routing support.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/espressif/uart_v1.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::espressif::uart_v1>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t, std::uint32_t) {
        // ROM-configured (115200 8N1); nothing to program in v1.
    }

    static void write(std::uint8_t byte) {
        // TX FIFO is 128 deep; leave headroom (validated threshold: 126).
        while (IP::txfifo_cnt.read(r()) >= 126u) {
        }
        r().FIFO = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxfifo_cnt.read(r()) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().FIFO);
        return true;
    }

    static void flush() {
        while (IP::txfifo_cnt.read(r()) != 0u) {
        }
    }

    // --- RX interrupt callback. RXFIFO_FULL (threshold 1) + RXFIFO_TOUT
    // (idle residue) cover both the byte-at-a-time and burst cases; the
    // ISR drains the FIFO FIRST and clears INT_CLR AFTER — a level source
    // left pending would livelock the level-1 dispatch loop. ---
    inline static void (*rx_fn)(void*, std::uint8_t) = nullptr;
    inline static void* rx_ctx = nullptr;

    static void rx_isr(void*) {
        while (IP::rxfifo_cnt.read(r()) != 0u) {
            const auto byte = static_cast<std::uint8_t>(r().FIFO);
            if (rx_fn != nullptr) {
                rx_fn(rx_ctx, byte);
            }
        }
        r().INT_CLR = IP::rxfifo_full_int_clr.mask | IP::rxfifo_tout_int_clr.mask |
                      IP::rxfifo_ovf_int_clr.mask;
    }

    static void enable_rx_irq(void (*fn)(void*, std::uint8_t), void* ctx) {
        rx_fn = fn;
        rx_ctx = ctx;
        alloy::irq::attach(Inst::irq, &rx_isr);
        // The boot ROM's uart_init leaves INT_ENA bits of its own — a
        // leftover source with a true condition would re-pend our level-1
        // line forever. Own the whole register: everything off, all
        // latches cleared, then exactly our two enables.
        r().INT_ENA = 0;
        r().INT_CLR = 0xFFFFFFFFu;
        // TRM: RXFIFO_FULL fires when cnt EXCEEDS the threshold (thrhd=1
        // -> at 2 bytes); a lone byte arrives via RX_TOUT ~10 byte-times
        // later (~0.9 ms at 115200) — imperceptible for an echo.
        IP::rxfifo_full_thrhd.write(r(), 1u);
        IP::rx_tout_thrhd.write(r(), 10u);
        IP::rx_tout_en.set(r());
        IP::rxfifo_full_int_ena.set(r());
        IP::rxfifo_tout_int_ena.set(r());
        alloy::irq::enable(Inst::irq);
    }

    static void disable_rx_irq() {
        IP::rxfifo_full_int_ena.clear(r());
        IP::rxfifo_tout_int_ena.clear(r());
        alloy::irq::detach(Inst::irq);
        rx_fn = nullptr;
    }
};

}  // namespace alloy::hal
