// UART driver for the ARM PL011 as integrated in the RP2040.
//
// BEHAVIOR only: base/gate/bits come from generated headers. Baud uses the
// pico-sdk rounding formula: div = 8*kernel/baud, IBRD = div>>7,
// FBRD = ((div & 0x7F) + 1) / 2. The old ecosystem hardcoded FBRD=53 where
// the formula gives 52 — computed here, never hardcoded.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/raspberrypi/uart_pl011.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::raspberrypi::uart_pl011>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t baud) {
        alloy::gate_on(Inst::gate);  // reset_release: waits RESET_DONE
        const std::uint32_t div = (8u * kernel_hz) / baud;
        std::uint32_t ibrd = div >> 7;
        std::uint32_t fbrd = ((div & 0x7Fu) + 1u) / 2u;
        if (ibrd == 0u) {
            ibrd = 1u;
            fbrd = 0u;
        } else if (ibrd >= 65535u) {
            ibrd = 65535u;
            fbrd = 0u;
        }
        r().UARTIBRD = ibrd;
        r().UARTFBRD = fbrd;
        // 8N1 + FIFOs. LCR_H write also latches the baud divisors.
        IP::wlen.write(r(), 0b11u);
        IP::fen.set(r());
        IP::uarten.set(r());
        IP::txe.set(r());
        IP::rxe.set(r());
    }

    static void write(std::uint8_t byte) {
        while (IP::txff.read(r()) != 0u) {
        }
        r().UARTDR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxfe.read(r()) != 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().UARTDR);
        return true;
    }

    static void flush() {
        while (IP::busy.read(r()) != 0u) {
        }
    }

    // --- RX interrupt callback. RTIM covers FIFO residue below the RX
    // trigger level; UARTICR shares the IMSC bit layout (PL011 TRM). ---
    inline static void (*rx_fn)(void*, std::uint8_t) = nullptr;
    inline static void* rx_ctx = nullptr;

    static void rx_isr(void*) {
        while (IP::rxfe.read(r()) == 0u) {
            const auto byte = static_cast<std::uint8_t>(r().UARTDR);
            if (rx_fn != nullptr) {
                rx_fn(rx_ctx, byte);
            }
        }
        r().UARTICR = IP::rxim.mask | IP::rtim.mask;
    }

    static void enable_rx_irq(void (*fn)(void*, std::uint8_t), void* ctx) {
        rx_fn = fn;
        rx_ctx = ctx;
        alloy::irq::attach(Inst::irq, &rx_isr);
        r().UARTIMSC = r().UARTIMSC | IP::rxim.mask | IP::rtim.mask;
        alloy::irq::enable(Inst::irq);
    }

    static void disable_rx_irq() {
        r().UARTIMSC = r().UARTIMSC & ~(IP::rxim.mask | IP::rtim.mask);
        alloy::irq::detach(Inst::irq);
        rx_fn = nullptr;
    }
};

}  // namespace alloy::hal
