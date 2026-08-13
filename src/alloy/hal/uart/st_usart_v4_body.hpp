// The programming sequence shared by ST's PRESC/FIFO UART family: the USART
// (st/usart_v4) and the LPUART (st/lpuart_v4).
//
// WHY THERE IS A SHARED BODY AT ALL. alloy's rule is one driver per IP
// version, and the LPUART earns its own IP file — two of the USART's registers
// do not exist on it, its BRR is four bits wider and means something else, and
// it has DE and Stop-mode wakeup fields the curated usart_v4 does not carry.
// But the register OFFSETS are identical, and every bit this bring-up sequence
// touches sits at the same position in both blocks. Copying the driver would
// have produced two files that drift: a fix to the ORE-wedge in the RX ISR, or
// to the DMA-TX done flag, would land in one of them.
//
// So the sequence lives here once, and the driver supplies what differs
// through three hooks (CRTP, so the calls are static and inline):
//
//   Impl::admit_rate(kernel_hz, baud)      what rates this block can reach.
//                                          Empty for the USART; the LPUART's
//                                          divisor has a WINDOW and rejects.
//   Impl::baud_div(kernel_hz, baud)        the divisor itself: `k/b` on the
//                                          USART, `(256*k)/b` on the LPUART.
//   Impl::program_vendor<O, De>()          the Layer-2 knobs whose register
//                                          fields only one of the two has.
//
// What is NOT a hook, deliberately: the frame (M1:M0, PCE/PS, STOP), the FIFO
// enable, byte I/O, the RX interrupt, and the DMA-TX handshake. Those are the
// same bits in the same order on both blocks, and a hook for each would have
// been the copy again with extra steps.

#pragma once

#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal::detail {

template <class Inst, class Impl>
struct st_usart_v4_body {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
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
        static_assert(O.data_bits >= 7u && O.data_bits <= 9u,
                      "this UART's M1:M0 encodes a 7-, 8- or 9-bit word");
        Impl::admit_rate(kernel_hz, c.baud);
        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());
        r().BRR = Impl::baud_div(kernel_hz, c.baud);

        // The parity bit lives INSIDE the word, so the M field is programmed
        // with data bits + parity, not with data bits.
        const bool pce = c.parity != hal::parity::none;
        const unsigned word = O.data_bits + (pce ? 1u : 0u);
        // THE ONE COMBINATION NEITHER LAYER CAN REJECT: data_bits is a
        // compile-time knob, parity is a runtime field, and 9 data bits plus
        // a parity bit is a 10-bit word this UART does not have. There is no
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

        Impl::template program_vendor<O, De>();

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
        Impl::isr_vendor();
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

}  // namespace alloy::hal::detail
