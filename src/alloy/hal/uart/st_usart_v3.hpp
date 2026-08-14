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
#include "alloy/hal/uart/st_usart_v4_body.hpp"  // st_usart_rx_dma_idle_body (shared, witnessed)
#include "alloy/hal/uart/uart_impl.hpp"
#include "alloy/ip/st/usart_v3.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

// LAYER 2 for this IP. Note what is here that is NOT in uart_opts<usart_v4>:
// hardware RS-485 driver-enable and line inversion. That asymmetry is not a
// judgement call — usart_v3's curated register data carries DEAT/DEDT/DEM/DEP
// and TXINV/RXINV/SWAP and usart_v4's does not, so v4 must not offer knobs it
// cannot program. And note what is NOT here that IS on v4: fifo_enable. This
// IP has no FIFOEN bit.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v3>
struct uart_opts<Inst> {
    //: Data bits, EXCLUDING parity — same name and same meaning as every
    //: other driver that has it (the cross-vendor naming rule).
    std::uint8_t data_bits = 8;
    bool invert_tx = false;   //: CR2.TXINV
    bool invert_rx = false;   //: CR2.RXINV
    bool swap_rx_tx = false;  //: CR2.SWAP
    //: DE lead/tail time in 16ths of a bit. The UNIT is a vendor register
    //: artefact (the field is five bits wide), which is exactly why it is a
    //: Layer-2 name and can never be promoted to alloy::uart::config.
    //:
    //: There is deliberately NO `de_enable` bool here. Driver-enable needs a
    //: PIN, so what turns it on is the binder tag `uart::de<pin>`; a bool
    //: that only works if some other code muxed a pin is the lie
    //: routes::routable<> exists to kill. These two are the timings only.
    std::uint8_t de_assert_16ths = 8;
    std::uint8_t de_deassert_16ths = 8;
};

// RX-via-DMA + IDLE come from st_usart_rx_dma_idle_body (st_usart_v4_body.hpp):
// this IP places CR1.IDLEIE, CR3.DMAR, ISR.IDLE, ICR.IDLECF/ORECF and RDR at
// the same offsets and bit positions as the v4 family — the same equivalence
// this file's header comment already claims for the bring-up subset — so the
// driver inherits the code the host IDLE witness (tests/test_st_usart_v4_idle
// .cpp) already exercises instead of copying it into a second, unwitnessed
// body. This is what folds uart.rx_ring()/write_dma open on the F7 boards
// (design §3.2 phase 3): the facade's rx_stream_capable gate probes exactly
// these members.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::usart_v3>
struct uart_impl<Inst> : detail::st_usart_rx_dma_idle_body<Inst> {
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

    // Layer 1 (runtime `c`) + Layer 2 (compile-time `O`). Same shape as the
    // v4 driver, plus the vendor knobs this IP's register data actually has.
    //
    // Runs on a just-gated peripheral (CR1/CR2/CR3 read as reset), so every
    // default-valued write is guarded and an unused knob emits nothing. The
    // UE-low/TEACK/REACK dance lives in configure_running() below, which is
    // what a LIVE reconfiguration needs and a cold enable does not.
    template <uart_opts<Inst> O = {}, bool De = false>
    static void enable(std::uint32_t kernel_hz, hal::uart_config c) {
        static_assert(O.data_bits >= 7u && O.data_bits <= 9u,
                      "this USART's M1:M0 encodes a 7-, 8- or 9-bit word");
        // The bound is the GENERATED field width, not a number typed here:
        // DEAT is five bits wide in the register data, so raw_mask is 31 and
        // the check cannot drift from the silicon description.
        static_assert(O.de_assert_16ths <= IP::deat.raw_mask,
                      "DE assertion time exceeds this USART's DEAT field width");
        static_assert(O.de_deassert_16ths <= IP::dedt.raw_mask,
                      "DE deassertion time exceeds this USART's DEDT field width");

        alloy::gate_on(Inst::gate);
        IP::ue.clear(r());
        r().BRR = baud_div(kernel_hz, c.baud);
        program_frame<O, De, true>(c);
        IP::te.set(r());
        IP::re.set(r());
        IP::ue.set(r());
    }

    // The frame/vendor writes, shared by enable() and configure_running().
    //
    // FromReset is the driver-authoring rule made mechanical: on a just-gated
    // peripheral every bit reads zero, so a default-valued write can be
    // SKIPPED — which is what makes an unused knob cost zero bytes. On a live
    // port nothing may be assumed and every field is written, including back
    // to its default, or a knob that was set by an earlier call stays set.
    template <uart_opts<Inst> O, bool De, bool FromReset>
    static void program_frame(hal::uart_config c) {
        const bool pce = c.parity != hal::parity::none;
        const unsigned word = O.data_bits + (pce ? 1u : 0u);
        // See st_usart_v4.hpp: 9 data bits plus parity is a 10-bit word this
        // USART does not have, and no static_assert can see a runtime parity.
        if constexpr (O.data_bits == 9u) {
            if (pce) {
                __builtin_trap();
            }
        }
        if constexpr (FromReset) {
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
        } else {
            IP::m0.write(r(), word == 9u ? 1u : 0u);
            IP::m1.write(r(), word == 7u ? 1u : 0u);
            IP::pce.write(r(), pce ? 1u : 0u);
            IP::ps.write(r(), c.parity == hal::parity::odd ? 1u : 0u);
            IP::stop.write(r(), c.stop == hal::stop_bits::two ? 2u : 0u);
        }

        if constexpr (O.invert_tx || !FromReset) {
            IP::txinv.write(r(), O.invert_tx ? 1u : 0u);
        }
        if constexpr (O.invert_rx || !FromReset) {
            IP::rxinv.write(r(), O.invert_rx ? 1u : 0u);
        }
        if constexpr (O.swap_rx_tx || !FromReset) {
            IP::swap.write(r(), O.swap_rx_tx ? 1u : 0u);
        }
        // Hardware driver-enable. Switched on by the PRESENCE of a
        // uart::de<pin> tag in the bind, never by a config bool — the pin is
        // muxed by the same decision that programs DEM, so the two cannot
        // disagree.
        if constexpr (De) {
            IP::deat.write(r(), O.de_assert_16ths);
            IP::dedt.write(r(), O.de_deassert_16ths);
            IP::dem.set(r());
            IP::dep.clear(r());  // active-high DE (the transceiver norm)
        } else if constexpr (!FromReset) {
            IP::dem.clear(r());
        }
    }

    // Reprogram a RUNNING port — the RS-485/multidrop path. UE must be LOW
    // while CR1/CR2/CR3/BRR change (RM0394 §38 marks them "only written when
    // UE=0"), and the re-enable is only complete when the hardware acks BOTH
    // directions — TEACK and REACK. A reconfigure that skips those waits and
    // transmits immediately ships its first byte at the OLD line shape. This
    // is what lets a running system change baud/parity from a protocol write:
    // send the ACK at the old settings, then call this.
    //
    // Unlike enable() it may NOT assume reset state, so it writes every field
    // unconditionally.
    template <uart_opts<Inst> O = {}, bool De = false>
    static void configure_running(std::uint32_t kernel_hz, hal::uart_config c) {
        IP::ue.clear(r());
        r().BRR = baud_div(kernel_hz, c.baud);
        program_frame<O, De, false>(c);
        IP::te.set(r());
        IP::re.set(r());
        IP::ue.set(r());
        while (IP::teack.read(r()) == 0u || IP::reack.read(r()) == 0u) {
        }
    }

    // DEPRECATED — the nine-field path. It is kept only so
    // `bind::reconfigure(serial_config)` keeps working for its stability
    // window; `configure_running<Opts>(kernel, config)` replaces it. This IP
    // is the one that could honour all nine fields, which is precisely why
    // the shared struct looked honest for as long as it did.
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
