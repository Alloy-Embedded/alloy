// UART driver for the ST usart_v3 IP (ISR/ICR era WITHOUT PRESC — F7/L4).
//
// The v4 driver's exact access pattern applies (same offsets and bit
// positions for the bring-up subset); this is a separate driver because the
// rule is one driver per IP version — v3 has no PRESC/FIFO to grow into.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/usart_v3.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v3>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // BRR (OVER8=0): round kernel/baud. Single source of truth — enable()
    // programs it and achieved_baud() inverts it, so the compile-time
    // tolerance check can never disagree with the hardware.
    static constexpr std::uint32_t baud_div(std::uint32_t kernel_hz, std::uint32_t baud) {
        return (kernel_hz + baud / 2u) / baud;
    }
    static constexpr alloy::frequency achieved_baud(std::uint32_t kernel_hz, std::uint32_t baud) {
        const std::uint32_t brr = baud_div(kernel_hz, baud);
        return alloy::frequency{brr != 0u ? kernel_hz / brr : 0u};
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t baud) {
        configure(kernel_hz, hal::serial_config{.baud = baud});
    }

    // Full line shape, runtime-safe: UE must be LOW while CR1/CR2/CR3/BRR
    // change (RM0394 §38 marks them "only written when UE=0"), and the
    // re-enable is only complete when the hardware acks BOTH directions —
    // TEACK and REACK. A reconfigure that skips those waits and transmits
    // immediately ships its first byte at the OLD line shape. This is what
    // lets a running system change baud/parity from a protocol write: send
    // the ACK at the old settings, then call this.
    static void configure(std::uint32_t kernel_hz, hal::serial_config c) {
        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());

        r().BRR = baud_div(kernel_hz, c.baud);

        // Parity lives in a 9th frame bit when enabled with 8 data bits:
        // M[1:0]=01 (9-bit frame) + PCE. data9 without parity is the raw
        // 9-bit frame some multidrop protocols use.
        const bool nine = c.data9 || c.parity != hal::parity::none;
        IP::m0.write(r(), nine ? 1u : 0u);
        IP::m1.write(r(), 0u);
        IP::pce.write(r(), c.parity != hal::parity::none ? 1u : 0u);
        IP::ps.write(r(), c.parity == hal::parity::odd ? 1u : 0u);
        IP::deat.write(r(), c.de_assert_16ths);
        IP::dedt.write(r(), c.de_deassert_16ths);

        IP::stop.write(r(), c.stop_bits >= 2u ? 2u : 0u);
        IP::txinv.write(r(), c.invert_tx ? 1u : 0u);
        IP::rxinv.write(r(), c.invert_rx ? 1u : 0u);

        IP::dem.write(r(), c.de_enable ? 1u : 0u);
        IP::dep.write(r(), 0u);  // active-high DE (the transceiver norm)

        IP::te.set(r());
        IP::re.set(r());
        IP::ue.set(r());
        while (IP::teack.read(r()) == 0u || IP::reack.read(r()) == 0u) {
        }
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

    // --- RX interrupt callback (same discipline as usart_v4: drain RDR,
    // clear ORE, user code never touches registers). ---
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
