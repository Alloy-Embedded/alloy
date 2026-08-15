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
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/raspberrypi/uart_pl011.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

// LAYER 2 for this IP. No `fifo_enable`: this driver has always turned the
// FIFOs on and turning them off is not a thing anyone has asked for, so the
// knob would be surface with no user. Adding it later is additive.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::raspberrypi::uart_pl011>
struct uart_opts<Inst> {
    //: Data bits, EXCLUDING parity — LCR_H.WLEN, {5, 6, 7, 8}. There is no
    //: 9-bit word on this IP, which is the other half of why data bits
    //: cannot be a Layer-1 field: the ST USARTs reach 9 and this does not.
    std::uint8_t data_bits = 8;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::raspberrypi::uart_pl011>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // Baud divisors (pico-sdk formula): div = 8*kernel/baud, IBRD = div>>7,
    // FBRD = ((div & 0x7F) + 1) / 2, clamped. Single source of truth — enable()
    // programs these and achieved_baud() inverts them, so the compile-time
    // tolerance check can never disagree with the hardware.
    struct baud_regs {
        std::uint32_t ibrd;
        std::uint32_t fbrd;
    };
    static constexpr baud_regs baud_div(std::uint32_t kernel_hz, std::uint32_t baud) {
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
        return {ibrd, fbrd};
    }
    static constexpr alloy::frequency achieved_baud(std::uint32_t kernel_hz, std::uint32_t baud) {
        const baud_regs d = baud_div(kernel_hz, baud);
        const std::uint32_t denom = 64u * d.ibrd + d.fbrd;  // BAUDDIV in 1/64ths
        return alloy::frequency{denom != 0u
                                    ? static_cast<std::uint32_t>(4ull * kernel_hz / denom)
                                    : 0u};
    }

    template <uart_opts<Inst> O = {}, bool De = false>
    static void enable(std::uint32_t kernel_hz, hal::uart_config c) {
        static_assert(!De,
                      "the PL011 has no hardware driver-enable; drive DE from a GPIO");
        static_assert(O.data_bits >= 5u && O.data_bits <= 5u + IP::wlen.raw_mask,
                      "LCR_H.WLEN encodes a 5- to 8-bit word on the PL011");
        alloy::gate_on(Inst::gate);  // reset_release: waits RESET_DONE
        const baud_regs d = baud_div(kernel_hz, c.baud);
        r().UARTIBRD = d.ibrd;
        r().UARTFBRD = d.fbrd;
        // Frame + FIFOs. The LCR_H write also latches the baud divisors, so
        // the whole line shape goes in with it. Parity is a bit OUTSIDE the
        // word here, unlike the ST USARTs — data_bits means data bits.
        IP::wlen.write(r(), O.data_bits - 5u);
        if (c.parity != hal::parity::none) {
            IP::pen.set(r());
            if (c.parity == hal::parity::even) {
                IP::eps.set(r());  // EPS=1 is EVEN on the PL011
            }
        }
        if (c.stop == hal::stop_bits::two) {
            IP::stp2.set(r());
        }
        IP::fen.set(r());
        IP::uarten.set(r());
        IP::txe.set(r());
        IP::rxe.set(r());
    }

    static void write(std::uint8_t byte) {
        using uartfr = typename IP::uartfr;
        while ((r().UARTFR & uartfr::txff) != 0u) {
        }
        r().UARTDR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        using uartfr = typename IP::uartfr;
        if ((r().UARTFR & uartfr::rxfe) != 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().UARTDR);
        return true;
    }

    static void flush() {
        using uartfr = typename IP::uartfr;
        while ((r().UARTFR & uartfr::busy) != 0u) {
        }
    }

    // --- TX via DMA. These three members are the whole gate: alloy::uart's
    // write_dma()/write_dma_begin()/write_dma_end() are constrained on them
    // existing plus a TX request id, and the id is the chip-wide
    // `Inst::dmareq_tx` (TREQ_SEL 20 for uart0), so a board with an RP2040 DMA
    // controller folds the DMA path open from board.json alone — no
    // preprocessor, and nothing here names a channel.
    //
    // WHY THE PL011 NEEDS NO PRE-CLEAR THE WAY THE ST USARTs DO. st_usart_v4's
    // dma_tx_begin() clears TC before raising DMAT, because on that IP TC is
    // sticky and dma_tx_end() waits on it. There is no TC here: completion is
    // read from UARTFR.BUSY, which is a LIVE level (transmitter or FIFO
    // non-empty), not a latched flag, so there is nothing stale to clear and a
    // clear would be a write invented to match another family's shape.
    static void dma_tx_begin() { IP::txdmae.set(r()); }

    // The §4 teardown order's UART half, and the honest completion the facade
    // documents: the channel finishing means the last byte reached UARTDR, not
    // that it left the pin. BUSY covers both the shift register and the TX
    // FIFO, so this spin is the "transmitter actually drained" wait, and it is
    // the same one flush() does — spelled again rather than shared, because
    // these are two different contracts that happen to agree today.
    static void dma_tx_end() {
        using uartfr = typename IP::uartfr;
        while ((r().UARTFR & uartfr::busy) != 0u) {
        }
        IP::txdmae.clear(r());
    }

    [[nodiscard]] static std::uintptr_t tdr_addr() {
        return reinterpret_cast<std::uintptr_t>(&r().UARTDR);
    }

    // --- RX via DMA: A NAMED ABSENCE, not an oversight. ---
    //
    // There is deliberately no dma_rx_begin()/dma_rx_end()/rdr_addr() and no
    // enable_idle_irq() here, so alloy::uart::rx_stream_capable is FALSE for
    // this IP and `uart.rx_ring()` is constrained away with a message naming
    // the missing hooks. Two independent reasons, either sufficient, and both
    // of them are about this family rather than about effort:
    //
    //  1. THERE IS NO RING BEHIND THE ROUTE. rx_stream_capable also demands
    //     dma::ring_capable, and raspberrypi_dma_v1_body.hpp sets
    //     supports_ring = false — this silicon has no half-transfer event
    //     anywhere and a single channel HALTS at the end of its count. Adding
    //     UART hooks would not make rx_ring() appear; it would only move the
    //     compile error to the other half of the same concept.
    //  2. THE FRAME-GAP EVENT IS UNSOURCED HERE. anchor 2.2's wake is the IDLE
    //     event, and the PL011's nearest analogue is RTIM, which asserts when
    //     the RX FIFO is NOT empty and the line then stays quiet. Whether that
    //     can fire at all while a DMA channel is draining the FIFO depends on
    //     which request flavour the RP2040 wires to TREQ_SEL 21 —
    //     UARTRXDMASREQ (any non-empty FIFO, which would keep it empty and RTIM
    //     silent) or UARTRXDMABREQ (a burst at the UARTIFLS level, which would
    //     leave residue for RTIM to see). The pinned SVD names both flavours in
    //     UARTDMACR/DMAONERR's own text and picks NEITHER, and this project does
    //     not curate silicon behaviour it cannot source. An enable_idle_irq()
    //     built on the guess would be a hook that compiles, arms, and may never
    //     fire — a sleeper that never wakes, which is worse than a compile
    //     error naming what is missing.
    //
    // RXDMAE and DMAONERR are curated (registers/raspberrypi/uart_pl011.yaml)
    // and unused, which is what a future RX path would be built from.

    // --- RX interrupt callback. RTIM covers FIFO residue below the RX
    // trigger level; UARTICR shares the IMSC bit layout (PL011 TRM). ---
    inline static void (*rx_fn)(void*, std::uint8_t) = nullptr;
    inline static void* rx_ctx = nullptr;

    static void rx_isr(void*) {
        using uartfr = typename IP::uartfr;
        while ((r().UARTFR & uartfr::rxfe) == 0u) {
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
        alloy::irq::detach(Inst::irq, &rx_isr);
        rx_fn = nullptr;
    }
};

}  // namespace alloy::hal
