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

    // --- RX via DMA + the IDLE line event (design §2.2's read side). DMAR
    // reroutes the RXNE request to the DMA fabric instead of the RXNE
    // interrupt; the caller arms the CHANNEL first, then calls dma_rx_begin()
    // — a request raised before the channel is armed is a byte the ring never
    // sees. Teardown is the reverse and it is the §4 order: dma_rx_end() stops
    // the request stream BEFORE the channel stops (the facade's destructor
    // sequence encodes it). ---
    static void dma_rx_begin() {
        // A set ORE freezes reception on this IP (the same wedge rx_isr
        // clears): entering DMA mode with it set would stall the request
        // stream before the first byte moved.
        if (IP::ore.read(r()) != 0u) {
            r().ICR = IP::orecf.mask;
        }
        IP::dmar.set(r());
    }

    static void dma_rx_end() { IP::dmar.clear(r()); }

    [[nodiscard]] static std::uintptr_t rdr_addr() {
        return reinterpret_cast<std::uintptr_t>(&r().RDR);
    }

    // --- The IDLE event: one interrupt per FRAME GAP (first idle bit time
    // after a byte), not one per byte — the wake that lets a Modbus consumer
    // sleep through a whole frame's worth of DMA'd bytes and read the ring
    // once (design §2.2's t3.5 story; the precise gap arithmetic stays the
    // protocol's, via uptime_us — IDLE fires at ONE character time).
    //
    // Same shared-line contract as rx_isr: return untouched unless our flag is
    // up, then clear it (level-triggered) and hand off. NO POLLED idle
    // ACCESSOR EXISTS — the ISR consumes the flag, so the first polled
    // consumer owes the latch and its witness (the half<Ch>() precedent in
    // st_dma_v1_body.hpp / test_st_dma_v1_latch.cpp), not a bare read. ---
    inline static void (*idle_fn)(void*) = nullptr;
    inline static void* idle_ctx = nullptr;
    inline static bool idle_armed = false;

    static void idle_isr(void*) {
        if (IP::idle.read(r()) == 0u) {
            return;
        }
        r().ICR = IP::idlecf.mask;
        if (idle_fn != nullptr) {
            idle_fn(idle_ctx);
        }
    }

    // fn == nullptr is a legitimate registration: the interrupt STILL fires
    // and wakes a WFI/WFE sleeper (that wake IS anchor 2.2's mechanism); the
    // ISR clears the flag so the line does not re-fire forever.
    //
    // Calling again WHILE ARMED is a LISTENER SWAP, not a re-arm — the
    // facade's contract (uart::rx_stream::on_idle: "rx_ring() armed the IDLE
    // interrupt already; registering here only chooses who is told"). The
    // swap must not repeat the arming sequence: a second irq::attach of the
    // same ISR on the same line is the alloy::irq duplicate trap, and
    // re-clearing IDLE here would eat a gap the already-armed ISR owns.
    // The (fn, ctx) pair is swapped under a masked window: with the line
    // armed and live, an IDLE landing between the two stores would call one
    // listener's fn with the other's ctx.
    static void enable_idle_irq(void (*fn)(void*), void* ctx) {
        {
            const arch::irq_state saved = arch::irq_save();
            idle_fn = fn;
            idle_ctx = ctx;
            arch::irq_restore(saved);
        }
        if (idle_armed) {
            return;
        }
        // Clear a stale IDLE first: the flag sets whenever the line has been
        // idle since the last byte — armed mid-silence it would fire
        // immediately and report a gap nobody waited through.
        r().ICR = IP::idlecf.mask;
        alloy::irq::attach(Inst::irq, &idle_isr);
        IP::idleie.set(r());
        alloy::irq::enable(Inst::irq);
        idle_armed = true;
    }

    static void disable_idle_irq() {
        IP::idleie.clear(r());
        alloy::irq::detach(Inst::irq, &idle_isr);
        idle_fn = nullptr;
        idle_ctx = nullptr;
        idle_armed = false;
    }
};

}  // namespace alloy::hal::detail
