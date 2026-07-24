// UART driver for the ST usart_v2 IP — the CLASSIC non-FIFO variant
// (F1/F2/F4/F7-lite/L1). This is a genuinely DIFFERENT register layout from
// usart_v3/v4, not a subset: CR1 is at 0x0C (not 0x00), status lives in a
// separate SR (0x00), and data is one read/write DR (0x04) — there is NO
// unified ISR/RDR/TDR/ICR. Enable bit UE is CR1 bit 13 (not bit 0).
//
// BRR is a 12.4 fixed-point USARTDIV (mantissa[15:4] + fraction[3:0]). With
// OVER8=0 (our default) the raw value = round(fck/baud) DIRECTLY — the low 4
// bits are the fraction, so writing fck/baud is correct and must NOT be
// masked. Status flags clear by read sequences, not a w1c ICR: RXNE clears
// on DR read, ORE by SR-read-then-DR-read, TXE by DR write.
//
// Not silicon-validated (no F4 board on hand) — tier-2, compile-checked.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/usart_v2.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v2>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t baud) {
        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());
        // OVER8=0: BRR is round(fck/baud) as a 12.4 fixed point — the low 4
        // bits ARE the fraction, so this whole value is written verbatim.
        r().BRR = (kernel_hz + baud / 2u) / baud;
        IP::te.set(r());
        IP::re.set(r());
        IP::ue.set(r());
    }

    static void write(std::uint8_t byte) {
        while (IP::txe.read(r()) == 0u) {
        }
        r().DR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        if (IP::rxne.read(r()) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().DR);  // reading DR clears RXNE
        return true;
    }

    static void flush() {
        while (IP::tc.read(r()) == 0u) {
        }
    }

    // --- RX interrupt callback. RXNE fires per byte; the ISR drains DR
    // (which clears RXNE) and clears a latched ORE via the SR-read-then-
    // DR-read sequence — a stuck ORE would wedge RX. User code never
    // touches registers. ---
    inline static void (*rx_fn)(void*, std::uint8_t) = nullptr;
    inline static void* rx_ctx = nullptr;

    static void rx_isr(void*) {
        const std::uint32_t sr = r().SR;  // sampled once (read is step 1 of ORE clear)
        if (sr & (IP::rxne.mask | IP::ore.mask)) {
            const auto byte = static_cast<std::uint8_t>(r().DR);  // step 2: clears RXNE+ORE
            if (rx_fn != nullptr) {
                rx_fn(rx_ctx, byte);
            }
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
        alloy::irq::detach(Inst::irq);
        rx_fn = nullptr;
    }
};

}  // namespace alloy::hal
