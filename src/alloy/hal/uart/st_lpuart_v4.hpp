// UART driver for the ST lpuart_v4 IP — the LOW-POWER UART of the PRESC/FIFO
// era (STM32G0/G4/L4/L5). Two of them on an STM32G0B1.
//
// It is the same block as st/usart_v4 minus GTPR and RTOR, so the programming
// sequence is shared verbatim (st_usart_v4_body.hpp) and this file supplies
// only the three things that genuinely differ:
//
//   1. THE DIVISOR. LPUARTDIV = (256 x f_ck) / baud, in a 20-bit BRR, and
//      RM0444 forbids values below 0x300. See st_lpuart_baud.hpp for the
//      arithmetic and for why the bounds are read from the chip database
//      rather than typed here.
//   2. RS-485 DRIVER-ENABLE, which works on this block and is a compile error
//      on a v4 USART — because alloy's curated lpuart_v4 data carries
//      DEM/DEP/DEAT/DEDT and its usart_v4 data does not. The asymmetry is the
//      database's, not a judgement about the silicon.
//   3. STOP-MODE WAKEUP, which is the reason the peripheral exists: with
//      CR1.UESM set the receiver stays clocked while the core sleeps, and
//      CR3.WUS picks what raises CR3.WUFIE's interrupt.
//
// WHAT NO ONE HAS WITNESSED. There is no G0B1 board on this bench, and Renode
// has no LPUART model for this die, so nothing below has moved a real line.
// The Stop-mode half is doubly unwitnessed: alloy has no low-power entry API
// at all yet, so `wake_from_stop` programs the peripheral to be wakeable and
// stops there — the WFI that would prove it is the caller's, and the PWR
// register work that selects Stop mode is not written.
//
// AND ONE THING THAT IS TRUE AND EASY TO MISS. The kernel clock alloy hands
// this driver is whatever the chip data's `kernel_clock` says, which on the
// G0 is PCLK — RCC_CCIPR's reset selection. alloy's clock_node enum has no
// spelling for LSE or HSI16, so the 32.768 kHz profile that makes an LPUART a
// low-power UART is NOT reachable from this data yet. At 64 MHz PCLK the
// divisor window puts the slowest reachable rate at 15 626 baud, so 9600 is
// impossible on this board today, and `admit_rate` below says so at compile
// time instead of programming a truncated BRR.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/types.hpp"
#include "alloy/core/units.hpp"
#include "alloy/hal/uart/st_lpuart_baud.hpp"
#include "alloy/hal/uart/st_usart_v4_body.hpp"
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/lpuart_v4.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

// What raises the wakeup while the core is in Stop. A Layer-2 vocabulary,
// because only an LPUART has it — the three reachable encodings of CR3.WUS
// plus the `off` that programs nothing. The values themselves are NOT written
// here: `program_vendor` reaches them through the generated CR3 flags enum, so
// the encoding stays in the curated data (registers/st/lpuart_v4.yaml).
enum class lpuart_wake : std::uint8_t {
    off,             //: CR1.UESM stays clear; the port stops with the core.
    address_match,   //: WUS = 00, and the address is CR2.ADD (not programmed here).
    start_bit,       //: WUS = 10 — wake on the first edge of any frame.
    rx_not_empty,    //: WUS = 11 — wake once a whole frame has landed in RDR.
};

// LAYER 2 for this IP. Two members are the same NAME and the same UNIT as
// uart_opts<usart_v3>'s, on purpose: portable code probes them by name
// (`if constexpr (requires { O{}.de_assert_16ths; })`) and a knob that meant
// 16ths on one driver and microseconds on another would make that probe a lie.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::lpuart_v4>
struct uart_opts<Inst> {
    //: Data bits, EXCLUDING parity. M1:M0 encodes a 7/8/9-bit word and the
    //: parity bit is part of that word, so the driver adds them up.
    std::uint8_t data_bits = 8;
    //: CR1.FIFOEN. Checked against Inst::feat::rx_fifo_depth, because
    //: "this IP has a FIFO" and "this instance has a FIFO" are different
    //: questions and only the chip database answers the second.
    bool fifo_enable = false;
    //: DE lead/tail time in 16ths of a bit (CR1.DEAT / CR1.DEDT). The UNIT is
    //: a vendor register artefact — the fields are five bits wide — which is
    //: exactly why it can never be promoted to alloy::uart::config. Only
    //: reached when the binder carries a `uart::de<pin>` tag; there is no
    //: `de_enable` bool, because driver-enable needs a PIN and a bool that
    //: works only if some other code muxed one is the lie routes::routable<>
    //: exists to kill.
    std::uint8_t de_assert_16ths = 8;
    std::uint8_t de_deassert_16ths = 8;
    //: Keep the receiver clocked in Stop mode and raise an interrupt on this
    //: event (CR1.UESM + CR3.WUS + CR3.WUFIE). The peripheral's reason to
    //: exist, and the half of it alloy cannot yet prove — see the header note.
    lpuart_wake wake_from_stop = lpuart_wake::off;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::lpuart_v4>
struct uart_impl<Inst> : detail::st_usart_v4_body<Inst, uart_impl<Inst>> {
    using IP = typename Inst::ip;
    using body = detail::st_usart_v4_body<Inst, uart_impl<Inst>>;

    //: The divisor window, both ends from the database and neither from here.
    //: The floor is the prose rule in RM0444, curated as a DEGREE; the ceiling
    //: is BRR's curated width, which the generated accessor already knows.
    static constexpr std::uint32_t brr_min = Inst::feat::brr_min;
    static constexpr std::uint32_t brr_max = IP::brr.raw_mask;

    static constexpr std::uint32_t baud_div(std::uint32_t kernel_hz, std::uint32_t baud) {
        return lpuart::divisor(kernel_hz, baud, brr_min, brr_max);
    }
    static constexpr alloy::frequency achieved_baud(std::uint32_t kernel_hz, std::uint32_t baud) {
        return alloy::frequency{lpuart::achieved_hz(kernel_hz, baud, brr_min, brr_max)};
    }

    // Does a divisor EXIST? This is the check the LPUART needs and a USART
    // does not: outside [3x, 4096x] of the baud rate there is no BRR value at
    // all, so `open({.baud = 9'600})` on a 64 MHz kernel is not an inaccurate
    // port, it is a port that would run at some unrelated rate. The named trap
    // is the guarantee; the compile error is a bonus that fires only when the
    // optimizer propagates the constant (measured matrix in admit.hpp — debug
    // builds do not get it). `open_checked<Baud>` is the net that always
    // fires, and its LPUART boundary is proven exact (15625 refused, 15626
    // accepted at 64 MHz). Same two-mechanism shape as
    // alloy::core::admit::uart_baud(); see admit.hpp for why the two lines are
    // spelled at the call site instead of wrapped.
    static void admit_rate(std::uint32_t kernel_hz, std::uint32_t baud) {
        const bool ok = lpuart::in_window(kernel_hz, baud, brr_min, brr_max);
        if (__builtin_constant_p(ok) && !ok) {
            alloy::core::admit::lpuart_baud_window();
        }
        if (!ok) {
            alloy::trap<alloy::trap_code::impossible_config>();
        }
    }

    // The compile-time half, offered to alloy::uart::bind::open_checked<Baud>
    // through a `requires`: a rate outside the window fails HERE, naming the
    // window, rather than at rate_check<Baud, 0, tol> which would only say the
    // achieved rate was zero.
    static constexpr bool baud_window_ok(std::uint32_t kernel_hz, std::uint32_t baud) {
        return lpuart::in_window(kernel_hz, baud, brr_min, brr_max);
    }

    // The Layer-2 knobs whose register fields only THIS block has. Runs with
    // UE clear on a just-gated peripheral, so CR1/CR3 read as reset and an
    // unset knob writes nothing.
    template <uart_opts<Inst> O, bool De>
    static void program_vendor() {
        // The bounds are the GENERATED field widths, not numbers typed here:
        // DEAT is five bits wide in the curated data, so raw_mask is 31 and
        // the check cannot drift from the silicon description.
        static_assert(O.de_assert_16ths <= IP::deat.raw_mask,
                      "DE assertion time exceeds this LPUART's DEAT field width");
        static_assert(O.de_deassert_16ths <= IP::dedt.raw_mask,
                      "DE deassertion time exceeds this LPUART's DEDT field width");

        std::uint32_t cr3 = 0u;
        if constexpr (De) {
            // DEP stays 0: DE active HIGH, which is what every RS-485
            // transceiver's DE input wants. Inverting it would need a pin-less
            // bool, and this driver does not offer one.
            cr3 |= static_cast<std::uint32_t>(IP::cr3::dem);
            IP::deat.write(body::r(), O.de_assert_16ths);
            IP::dedt.write(body::r(), O.de_deassert_16ths);
        }
        if constexpr (O.wake_from_stop != lpuart_wake::off) {
            cr3 |= static_cast<std::uint32_t>(IP::cr3::wufie) | wus_bits<O.wake_from_stop>();
            IP::uesm.set(body::r());
        }
        if constexpr (De || O.wake_from_stop != lpuart_wake::off) {
            body::r().CR3 = cr3;
        }
    }

    // WUF is level-ish: left set with WUFIE on, it re-enters the handler
    // forever. The shared RX ISR calls this after draining RDR, so a woken
    // port acknowledges its own wakeup without user code touching a register.
    static void isr_vendor() {
        if (IP::wuf.read(body::r()) != 0u) {
            body::r().ICR = IP::wucf.mask;
        }
    }

private:
    // The CR3.WUS encoding, taken from the generated flags enum — which is
    // where the curated `values:` map lands — so this driver never spells the
    // numbers 0/2/3. `wus_00` is a real encoding (address match) and not a
    // "nothing", which is why `off` is handled by the caller and not here.
    template <lpuart_wake W>
    static constexpr std::uint32_t wus_bits() {
        if constexpr (W == lpuart_wake::address_match) {
            return static_cast<std::uint32_t>(IP::cr3::wus_address_match);
        } else if constexpr (W == lpuart_wake::start_bit) {
            return static_cast<std::uint32_t>(IP::cr3::wus_start_bit);
        } else {
            return static_cast<std::uint32_t>(IP::cr3::wus_rxne);
        }
    }
};

}  // namespace alloy::hal

namespace alloy::uart {
// Spelled where the rest of the UART vocabulary is, so a call site reads
// `alloy::uart::lpuart_wake::start_bit` next to `alloy::uart::parity::even`.
// The NAME keeps its vendor prefix because the knob is Layer 2 and only one
// family's silicon has it — see docs/reference/peripheral-surface.md.
using lpuart_wake = alloy::hal::lpuart_wake;
}  // namespace alloy::uart
