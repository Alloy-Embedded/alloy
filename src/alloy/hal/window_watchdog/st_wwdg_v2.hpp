// Window-watchdog driver for the STM32 WWDG v2 (G0/G4/L4/F7 — three-bit
// WDGTB). The arithmetic lives in st_wwdg_detail.hpp so a host test can reach
// it; this file is the register sequence and nothing else.
//
// Three things here are not obvious from the register map, all three from
// RM0444's 'System window watchdog (WWDG)' chapter:
//
//  - CFR is programmed BEFORE CR. Setting CR.WDGA starts the countdown, and
//    WDGA is cleared only by a reset — there is no disable — so a window
//    written afterwards is written to a dog already running.
//  - the refresh writes CR with the arm bit CLEAR (0x40..0x7F, no WDGA).
//    That is what ST's own HAL_WWDG_Refresh does, and it is safe because WDGA
//    ignores a zero: software cannot switch this peripheral off.
//  - SR.EWIF is cleared by writing ZERO to it (rc_w0), not one. The reflexive
//    w1c leaves the flag set and re-enters the handler forever.
//
// The reload value is STATE, which no other watchdog in this tree has. The
// IWDG reloads from RLR on a key write; the WWDG's refresh IS the reload, so
// the value has to be remembered between start() and feed(). One byte per
// instance, in .data, and only for an instance a program actually starts.
//
// NOT SILICON-WITNESSED. No board was on hand, and Renode 1.16.1 ships no
// WWDG model for any STM32 (its Timers.STM32_IndependentWatchdog is the other
// peripheral), so nothing here has been observed to reset anything. What IS
// checked is the arithmetic, on the host, against the manual's formula.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/window_watchdog/st_wwdg_detail.hpp"
#include "alloy/hal/window_watchdog/wwdt_impl.hpp"
#include "alloy/ip/st/wwdg_v2.hpp"
#include "alloy/irq.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::wwdg_v2>
struct wwdt_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // DEGREE from the data, not a literal 7: wwdg_v1's WDGTB is two bits.
    static constexpr std::uint32_t timebase_max = Inst::feat::timebase_max;
    static_assert(timebase_max <= IP::wdgtb.raw_mask,
                  "st/wwdg: feat.timebase_max is larger than CFR.WDGTB can hold — "
                  "the IP's own field width and its recorded degree disagree, and "
                  "the driver would program a prescaler the silicon truncates");

    static constexpr std::uint32_t longest_us(std::uint32_t pclk_hz) {
        return detail::wwdg_longest_us(pclk_hz, timebase_max);
    }
    static constexpr std::uint32_t shortest_us(std::uint32_t pclk_hz) {
        return detail::wwdg_shortest_us(pclk_hz);
    }
    static constexpr detail::wwdg_program plan(std::uint32_t pclk_hz,
                                               std::uint32_t deadline_us,
                                               std::uint32_t earliest_us) {
        return detail::wwdg_plan(pclk_hz, timebase_max, deadline_us, earliest_us);
    }

    static detail::wwdg_program start(std::uint32_t pclk_hz, std::uint32_t deadline_us,
                                      std::uint32_t earliest_us) {
        alloy::gate_on(Inst::gate);
        const detail::wwdg_program p = plan(pclk_hz, deadline_us, earliest_us);
        reload_ = p.reload;

        // Whole-word write: every bit of CFR belongs to this one decision, so
        // there is nothing to preserve and a read-modify-write would only
        // carry a reset value forward. EWI goes in only when a hook is
        // installed — an enabled interrupt with no handler is a vector to
        // Default_Handler one tick before the reset.
        std::uint32_t cfr = (static_cast<std::uint32_t>(p.win) << IP::w.pos) |
                            (static_cast<std::uint32_t>(p.wdgtb) << IP::wdgtb.pos);
        if (hook_ != nullptr) {
            cfr |= IP::ewi.mask;
        }
        r().CFR = cfr;

        // And the countdown starts HERE, on this store.
        r().CR = detail::wwdg_cr_wdga | static_cast<std::uint32_t>(p.reload);
        return p;
    }

    static void feed() { r().CR = reload_; }

    // The last-gasp hook: EWI fires when the counter reaches 0x40, one tick
    // before the reset. Install it BEFORE start() — CFR.EWI, like WDGA, is
    // cleared only by a reset, so this cannot be turned on later without
    // rewriting CFR under a running dog.
    static void on_early_wakeup(void (*fn)()) {
        hook_ = fn;
        if (fn != nullptr && !attached_) {
            alloy::irq::attach(Inst::irq, &early_isr);
            alloy::irq::enable(Inst::irq);
            attached_ = true;
        }
    }

private:
    static void early_isr(void*) {
        // Shared-vector contract (alloy/irq.hpp): a safe no-op when this
        // peripheral has nothing pending.
        if (IP::ewif.read(r()) == 0u) {
            return;
        }
        r().SR = 0u;  // rc_w0 — writing the bit back would NOT clear it
        if (hook_ != nullptr) {
            hook_();
        }
    }

    static inline std::uint8_t reload_ = 0x7Fu;
    static inline void (*hook_)() = nullptr;
    static inline bool attached_ = false;
};

}  // namespace alloy::hal
