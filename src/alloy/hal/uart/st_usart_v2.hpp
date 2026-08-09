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
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/usart_v2.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

// LAYER 2 for this IP. One knob, and its DOMAIN is narrower than v3/v4's for
// a reason the register data states outright: this generation has a single M
// bit, not M1:M0, so the word is 8 or 9 bits and 7 is not expressible. That
// narrower domain is exactly why data bits cannot be a Layer-1 field.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v2>
struct uart_opts<Inst> {
    //: Data bits, EXCLUDING parity. {8, 9} on this IP.
    std::uint8_t data_bits = 8;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v2>
struct uart_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // BRR (OVER8=0): round(fck/baud) as a 12.4 fixed point — the low 4 bits ARE
    // the fraction, so the whole value is written verbatim. Single source of
    // truth — enable() programs it and achieved_baud() inverts it, so the
    // compile-time tolerance check can never disagree with the hardware.
    static constexpr std::uint32_t baud_div(std::uint32_t kernel_hz, std::uint32_t baud) {
        return (kernel_hz + baud / 2u) / baud;
    }
    static constexpr alloy::frequency achieved_baud(std::uint32_t kernel_hz, std::uint32_t baud) {
        const std::uint32_t brr = baud_div(kernel_hz, baud);
        return alloy::frequency{brr != 0u ? kernel_hz / brr : 0u};
    }

    // Layer 1 (runtime `c`) + Layer 2 (compile-time `O`). Runs on a just-gated
    // peripheral, so every default-valued write is guarded and an unused knob
    // costs nothing.
    template <uart_opts<Inst> O = {}, bool De = false>
    static void enable(std::uint32_t kernel_hz, hal::uart_config c) {
        static_assert(!De,
                      "this USART generation has no hardware driver-enable (no DEM bit); drive DE from a GPIO");
        static_assert(O.data_bits >= 8u && O.data_bits <= 9u,
                      "this USART generation has a single M bit: the word is "
                      "8 or 9 bits, and 7 is not expressible");
        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());
        r().BRR = baud_div(kernel_hz, c.baud);

        // Parity occupies a bit of the M word, exactly as on v3/v4.
        const bool pce = c.parity != hal::parity::none;
        if constexpr (O.data_bits == 9u) {
            if (pce) {
                __builtin_trap();  // 9 data bits + parity is a 10-bit word
            }
        }
        if (O.data_bits + (pce ? 1u : 0u) == 9u) {
            IP::m.set(r());
        }
        if (pce) {
            IP::pce.set(r());
            if (c.parity == hal::parity::odd) {
                IP::ps.set(r());
            }
        }
        if (c.stop == hal::stop_bits::two) {
            IP::stop.write(r(), 2u);  // CR2.STOP: 00=1, 10=2
        }

        IP::te.set(r());
        IP::re.set(r());
        IP::ue.set(r());
    }

    static void write(std::uint8_t byte) {
        using sr = typename IP::sr;
        while ((r().SR & sr::txe) == 0u) {
        }
        r().DR = byte;
    }

    [[nodiscard]] static bool read(std::uint8_t& byte) {
        using sr = typename IP::sr;
        if ((r().SR & sr::rxne) == 0u) {
            return false;
        }
        byte = static_cast<std::uint8_t>(r().DR);  // reading DR clears RXNE
        return true;
    }

    static void flush() {
        using sr = typename IP::sr;
        while ((r().SR & sr::tc) == 0u) {
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
        if (sr & (IP::sr::rxne | IP::sr::ore)) {
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
        alloy::irq::detach(Inst::irq, &rx_isr);
        rx_fn = nullptr;
    }
};

}  // namespace alloy::hal
