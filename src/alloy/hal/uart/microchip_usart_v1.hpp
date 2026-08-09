// UART driver for the Microchip SAM full USART (usart_v1: E70-era), async 8N1.
//
// BEHAVIOR only: every base/bit comes from the generated IP header and
// instance descriptor. The MR write is explicit about MODE/USCLKS/CHRL/PAR —
// the old ecosystem drove this IP through the plain-UART model and silently
// got 5-bit characters (CHRL=0); the field masks below make that mistake
// impossible to repeat quietly.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/microchip/usart_v1.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

// LAYER 2 for this IP.
//
// THE FALSIFICATION TEST, and its result. The design's naming rule says the
// same silicon feature must get the same member name and unit in every driver
// that has it, and the test named for it was: implement RS-485 DE on both
// st_usart_v3 and microchip_usart_v1, and see whether `de_assert_16ths`
// survives. It does not, and the reason is silicon, not naming. The SAM USART
// has no DE assert/deassert time at all: RS-485 mode asserts RTS around the
// frame automatically and the only tunable is US_TTGR, a transmitter time
// GUARD measured in whole bit periods after the stop bit. Neither MODE=RS485
// nor TTGR is in alloy's curated register data, so this driver cannot offer a
// DE knob under any name.
//
// So Layer 2 is a DESCRIPTION, not a contract: `libs/` code may probe an opts
// member by name, but it may not assume that a feature present on one vendor
// appears under the same name on another, because often it does not appear at
// all. Recorded in docs/reference/peripheral-surface.md.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::usart_v1>
struct uart_opts<Inst> {
    //: Data bits, EXCLUDING parity — MR.CHRL, {5, 6, 7, 8}. Parity is a
    //: separate bit on this IP, unlike the ST USARTs where it eats one of
    //: the word's bits. Same name, same meaning; the DOMAIN differs, which
    //: is why data bits is not a Layer-1 field anywhere.
    std::uint8_t data_bits = 8;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::usart_v1>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // Baud divisor (CD): fixed 16x oversampling, round to nearest. Single
    // source of truth — enable() programs it and achieved_baud() inverts it,
    // so the compile-time tolerance check can never disagree with the hardware.
    static constexpr std::uint32_t baud_div(std::uint32_t kernel_hz, std::uint32_t baud) {
        return (kernel_hz + 8u * baud) / (16u * baud);
    }
    static constexpr alloy::frequency achieved_baud(std::uint32_t kernel_hz, std::uint32_t baud) {
        const std::uint32_t cd = baud_div(kernel_hz, baud);
        return alloy::frequency{cd != 0u ? kernel_hz / (16u * cd) : 0u};
    }

    template <uart_opts<Inst> O = {}, bool De = false>
    static void enable(std::uint32_t kernel_hz, hal::uart_config c) {
        using cr = typename IP::cr;
        using mr = typename IP::mr;
        static_assert(!De,
                      "the SAM USART has no DE assert/deassert time and alloy's data models neither MODE=RS485 nor TTGR; drive DE from a GPIO");
        static_assert(O.data_bits >= 5u && O.data_bits <= 5u + IP::chrl.raw_mask,
                      "MR.CHRL encodes a 5- to 8-bit character on this USART");
        alloy::gate_on(Inst::gate);
        // CR is write-only: whole-value command writes.
        r().CR = cr::rstrx | cr::rsttx | cr::rxdis | cr::txdis | cr::rststa;
        // MR: normal mode, MCK, 8-bit, no parity. WHOLE-value write, not a
        // field-by-field RMW: on the SAM E70/S70/V71 USART, US_MR does NOT
        // reset to zero the way the datasheet claims — its top two bits
        // (MODSYNC and ONEBIT) come up set (verified on silicon at reset). An
        // RMW field-init would preserve ONEBIT, which redefines the async
        // start-frame delimiter and mangles every byte on the wire. Composing
        // the whole word from named flags writes exactly those bits, clearing
        // the stale ones — and reads like the register table.
        //
        // The frame shape joins that whole-value write rather than following
        // it with read-modify-writes: US_MR's stale reset bits are the reason
        // the composition exists, and an RMW after it would put them back.
        // Every term below is a constant or a two-way select, so the word is
        // still one store.
        r().MR = mr::mode_normal | mr::usclks_mck | chrl_value<O.data_bits>() |
                 par_value(c.parity) | nbstop_value(c.stop);
        IP::cd.write(r(), baud_div(kernel_hz, c.baud));
        r().CR = cr::rxen | cr::txen;
    }

    template <std::uint8_t Bits>
    static constexpr std::uint32_t chrl_value() {
        using mr = typename IP::mr;
        if constexpr (Bits == 5u) {
            return static_cast<std::uint32_t>(mr::chrl_five);
        } else if constexpr (Bits == 6u) {
            return static_cast<std::uint32_t>(mr::chrl_six);
        } else if constexpr (Bits == 7u) {
            return static_cast<std::uint32_t>(mr::chrl_seven);
        } else {
            return static_cast<std::uint32_t>(mr::chrl_eight);
        }
    }

    static constexpr std::uint32_t par_value(hal::parity p) {
        using mr = typename IP::mr;
        switch (p) {
            case hal::parity::even: return static_cast<std::uint32_t>(mr::par_even);
            case hal::parity::odd: return static_cast<std::uint32_t>(mr::par_odd);
            case hal::parity::none: break;
        }
        return static_cast<std::uint32_t>(mr::par_none);
    }

    // MR.NBSTOP: 0 = 1 stop bit, 2 = 2 stop bits (1 = 1.5, which alloy's
    // Layer-1 stop_bits deliberately cannot express).
    static constexpr std::uint32_t nbstop_value(hal::stop_bits s) {
        return s == hal::stop_bits::two ? (2u << IP::nbstop.pos) : 0u;
    }

    static void write(std::uint8_t byte) {
        while (IP::txrdy.read(r()) == 0u) {
        }
        r().THR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxrdy.read(r()) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().RHR);
        return true;
    }

    static void flush() {
        while (IP::txempty.read(r()) == 0u) {
        }
    }

    // --- RX interrupt callback. IER/IDR mirror the CSR bit layout (SAM
    // USART TRM), so the CSR field masks drive the write-only enables. ---
    inline static void (*rx_fn)(void*, std::uint8_t) = nullptr;
    inline static void* rx_ctx = nullptr;

    static void rx_isr(void*) {
        while (IP::rxrdy.read(r()) != 0u) {
            const auto byte = static_cast<std::uint8_t>(r().RHR);
            if (rx_fn != nullptr) {
                rx_fn(rx_ctx, byte);
            }
        }
    }

    static void enable_rx_irq(void (*fn)(void*, std::uint8_t), void* ctx) {
        rx_fn = fn;
        rx_ctx = ctx;
        alloy::irq::attach(Inst::irq, &rx_isr);
        r().IER = IP::rxrdy.mask;
        alloy::irq::enable(Inst::irq);
    }

    static void disable_rx_irq() {
        r().IDR = IP::rxrdy.mask;
        alloy::irq::detach(Inst::irq, &rx_isr);
        rx_fn = nullptr;
    }

    // --- TX via XDMAC: no peripheral-side enable exists on SAME70 (the
    // PDC was dropped); TXRDY drives the request line selected by PERID.
    // The honest done-flag is TXEMPTY (shift register drained). ---
    static void dma_tx_begin() {}

    static void dma_tx_end() {
        while (IP::txempty.read(r()) == 0u) {
        }
    }

    [[nodiscard]] static std::uintptr_t tdr_addr() {
        return reinterpret_cast<std::uintptr_t>(&r().THR);
    }
};

}  // namespace alloy::hal
