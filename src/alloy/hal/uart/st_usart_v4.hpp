// UART driver for the ST usart_v4 IP (USART with PRESC/FIFO — G0/G4/H7/L5 era).
//
// BEHAVIOR only: every address, offset and field position comes from the
// generated alloy::ip::st::usart_v4 header and the generated instance
// descriptor. Blocking byte I/O + RX-interrupt callback.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/usart_v4.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

// LAYER 2 for this IP. Every member below names a register field this IP's
// generated header actually has; a knob the v4 data does not model is not
// here, and asking for it is a compile error that names the instance.
//
// Deliberately ABSENT, and it is the register data that says so, not an
// opinion: usart_v4's curated register set carries no DEAT/DEDT/DEM/DEP and
// no TXINV/RXINV/SWAP (usart_v3's does). The G0 silicon has them; alloy's
// database has not mined them yet. Until it does, a v4 driver cannot honour
// RS-485 DE, so `de_enable` must not be typeable here — which is the whole
// point of declaring Layer 2 beside the driver instead of in one shared
// struct.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v4>
struct uart_opts<Inst> {
    //: Data bits, EXCLUDING parity. M1:M0 encodes a 7/8/9-bit word and the
    //: parity bit is part of that word, so the driver adds them up.
    std::uint8_t data_bits = 8;
    //: CR1.FIFOEN. Checked against Inst::feat::rx_fifo_depth, because
    //: "this IP has a FIFO" and "this instance has a FIFO" are different
    //: questions and only the chip database answers the second.
    bool fifo_enable = false;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v4>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // BRR (OVER8=0, PRESC=0): round kernel/baud. Single source of truth —
    // enable() programs it and achieved_baud() inverts it, so the compile-time
    // tolerance check can never disagree with the hardware.
    static constexpr std::uint32_t baud_div(std::uint32_t kernel_hz, std::uint32_t baud) {
        return (kernel_hz + baud / 2u) / baud;
    }
    static constexpr alloy::frequency achieved_baud(std::uint32_t kernel_hz, std::uint32_t baud) {
        const std::uint32_t brr = baud_div(kernel_hz, baud);
        return alloy::frequency{brr != 0u ? kernel_hz / brr : 0u};
    }

    // Layer 1 (runtime `c`) + Layer 2 (compile-time `O`) in ONE entry point,
    // so there is one programming sequence rather than two that can drift.
    //
    // enable() runs on a just-gated peripheral, so CR1/CR2/CR3 read as reset
    // (all zero) and every default-valued write is guarded: a knob nobody set
    // contributes no instruction. Values that come from a literal config at
    // the call site fold away too — the `if`s below are runtime only when the
    // caller's config is.
    template <uart_opts<Inst> O = {}, bool De = false>
    static void enable(std::uint32_t kernel_hz, hal::uart_config c) {
        static_assert(!De,
                      "this USART's driver has no hardware driver-enable: alloy's curated usart_v4 register data carries no DEM/DEAT/DEDT (the silicon has them; the database has not mined them). Drive DE from a GPIO, or reach the registers through alloy::dev::");
        static_assert(O.data_bits >= 7u && O.data_bits <= 9u,
                      "this USART's M1:M0 encodes a 7-, 8- or 9-bit word");
        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());
        r().BRR = baud_div(kernel_hz, c.baud);

        // The parity bit lives INSIDE the word, so the M field is programmed
        // with data bits + parity, not with data bits.
        const bool pce = c.parity != hal::parity::none;
        const unsigned word = O.data_bits + (pce ? 1u : 0u);
        // THE ONE COMBINATION NEITHER LAYER CAN REJECT: data_bits is a
        // compile-time knob, parity is a runtime field, and 9 data bits plus
        // a parity bit is a 10-bit word this USART does not have. There is no
        // static_assert that can see a runtime parity, and silently programming
        // 8 bits would be the exact lie this design exists to remove — so it
        // traps, like the double-open guard in alloy/uart.hpp. Costs nothing
        // unless you asked for 9 data bits.
        if constexpr (O.data_bits == 9u) {
            if (pce) {
                __builtin_trap();
            }
        }
        if (word == 9u) {
            IP::m0.set(r());
        } else if (word == 7u) {
            IP::m1.set(r());
        }
        if (pce) {
            IP::pce.set(r());
            if (c.parity == hal::parity::odd) {
                IP::ps.set(r());
            }
        }
        if (c.stop == hal::stop_bits::two) {
            IP::stop.write(r(), 2u);
        }

        if constexpr (O.fifo_enable) {
            static_assert(Inst::feat::rx_fifo_depth > 0u,
                          "this instance has no RX FIFO (feat::rx_fifo_depth is 0)");
            IP::fifoen.set(r());
        }

        IP::te.set(r());
        IP::re.set(r());
        IP::ue.set(r());
    }

    static void write(std::uint8_t byte) {
        while (IP::txe.read(r()) == 0u) {
        }
        r().TDR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxne.read(r()) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().RDR);
        return true;
    }

    static void flush() {
        while (IP::tc.read(r()) == 0u) {
        }
    }

    // --- RX interrupt callback (the driver ISR does ALL register work:
    // drain RDR, clear ORE — a set ORE wedges RX otherwise — then hand each
    // byte to the user function; user code never touches registers). ---
    inline static void (*rx_fn)(void*, std::uint8_t) = nullptr;
    inline static void* rx_ctx = nullptr;

    static void rx_isr(void*) {
        while (IP::rxne.read(r()) != 0u) {
            const auto byte = static_cast<std::uint8_t>(r().RDR);
            if (rx_fn != nullptr) {
                rx_fn(rx_ctx, byte);
            }
        }
        if (IP::ore.read(r()) != 0u) {
            r().ICR = IP::orecf.mask;
        }
    }

    static void enable_rx_irq(void (*fn)(void*, std::uint8_t), void* ctx) {
        rx_fn = fn;
        rx_ctx = ctx;
        alloy::irq::attach(Inst::irq, &rx_isr);
        IP::rxneie.set(r());
        alloy::irq::enable(Inst::irq);
    }

    static void disable_rx_irq() {
        IP::rxneie.clear(r());
        alloy::irq::detach(Inst::irq, &rx_isr);
        rx_fn = nullptr;
    }

    // --- TX via DMA: DMA TCIF only means the last byte reached TDR; the
    // honest done-flag is ISR.TC (shift register + FIFO drained), so the
    // stale TC is cleared at begin and polled at end (RM0444 DMA-TX
    // procedure). 8-bit frames only: APB lane duplication can at worst
    // touch TDR[8], which the transmitter ignores when M=00. ---
    static void dma_tx_begin() {
        r().ICR = IP::tccf.mask;
        IP::dmat.set(r());
    }

    static void dma_tx_end() {
        while (IP::tc.read(r()) == 0u) {
        }
        IP::dmat.clear(r());
    }

    [[nodiscard]] static std::uintptr_t tdr_addr() {
        return reinterpret_cast<std::uintptr_t>(&r().TDR);
    }
};

}  // namespace alloy::hal
