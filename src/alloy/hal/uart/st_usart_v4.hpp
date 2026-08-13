// UART driver for the ST usart_v4 IP (USART with PRESC/FIFO — G0/G4/H7/L5 era).
//
// BEHAVIOR only: every address, offset and field position comes from the
// generated alloy::ip::st::usart_v4 header and the generated instance
// descriptor. Blocking byte I/O + RX-interrupt callback.
//
// The programming sequence itself lives in st_usart_v4_body.hpp, shared with
// the LPUART driver (st_lpuart_v4.hpp) — same offsets, same bit positions for
// every field the sequence touches. What this file supplies is what differs:
// the divisor, and the Layer-2 knobs whose register fields THIS block has.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/st_usart_v4_body.hpp"
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
// struct. (The LPUART's curated data DOES carry them, so the same tag that
// is a compile error here works there. The asymmetry is the database's.)
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
struct uart_impl<Inst> : detail::st_usart_v4_body<Inst, uart_impl<Inst>> {
    using IP = typename Inst::ip;

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

    // A plain divider reaches every rate below its kernel clock, so there is
    // nothing to admit beyond what alloy::uart::open() already checks. The
    // LPUART overrides this because its divisor has a floor as well as a
    // ceiling; see st_lpuart_v4.hpp.
    static void admit_rate(std::uint32_t, std::uint32_t) {}

    // The Layer-2 hook. This block's only vendor knob (fifo_enable) is
    // programmed by the shared body, so all that is left is the tag this
    // driver must REFUSE.
    template <uart_opts<Inst> O, bool De>
    static void program_vendor() {
        static_assert(!De,
                      "this USART's driver has no hardware driver-enable: alloy's curated usart_v4 register data carries no DEM/DEAT/DEDT (the silicon has them; the database has not mined them). Drive DE from a GPIO, use an LPUART on this die (its curated data does carry them), or reach the registers through alloy::dev::");
        (void)O;
    }

    // Nothing this block raises needs clearing beyond ORE, which the shared
    // RX ISR already does.
    static void isr_vendor() {}
};

}  // namespace alloy::hal
