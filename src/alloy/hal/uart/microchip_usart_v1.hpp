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
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/microchip/usart_v1.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::usart_v1>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t baud) {
        alloy::gate_on(Inst::gate);
        // CR is write-only: whole-value command writes composed from field masks.
        r().CR = IP::rstrx.mask | IP::rsttx.mask | IP::rxdis.mask |
                 IP::txdis.mask | IP::rststa.mask;
        // MR: normal mode, MCK, 8-bit, no parity. WHOLE-value write, not a
        // field-by-field RMW: on the SAM E70/S70/V71 USART, US_MR does NOT
        // reset to zero the way the datasheet claims — its top two bits
        // (MODSYNC and ONEBIT) come up set (verified on silicon at reset). An
        // RMW field-init would preserve ONEBIT, which redefines the async
        // start-frame delimiter and mangles every byte on the wire. Composing
        // the whole word from the field positions clears those stale bits.
        r().MR = (0u << IP::mode.pos) | (0u << IP::usclks.pos) |
                 (3u << IP::chrl.pos) | (4u << IP::par.pos);
        // Fixed 16x oversampling: CD = round(kernel / (16 * baud)).
        IP::cd.write(r(), (kernel_hz + 8u * baud) / (16u * baud));
        r().CR = IP::rxen.mask | IP::txen.mask;
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
        alloy::irq::detach(Inst::irq);
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
