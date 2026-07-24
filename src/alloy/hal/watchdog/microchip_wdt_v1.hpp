// Watchdog driver for the SAM E70/S70/V71 WDT (also the RSWDT instance).
//
// BEHAVIOR only: base and register/field layout come from generated data. The
// WDT runs off the always-on slow clock (SLCK/128 = 256 Hz) — no gate. US_MR
// is WRITE-ONCE: when a board declares the watchdog role, codegen SKIPS the
// bring-up disable so this start() is the first (and only) MR write; feed()
// then just restarts the counter. WDDBGHLT/WDIDLEHLT are set so the dog does
// not bite while the core is halted in a debugger or idle.

#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/hal/watchdog/wdt_impl.hpp"
#include "alloy/ip/microchip/wdt_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::wdt_v1>
struct wdt_impl<Inst> {
    using IP = typename Inst::ip;
    static constexpr std::uint32_t slow_hz = 256;   // SLCK 32768 / 128
    static constexpr std::uint32_t max_wdv = 0xFFFu;  // 12-bit counter (~16 s)

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // Configure the write-once MR: reset the MCU on underflow after `timeout`,
    // feed allowed at any time (WDD max), frozen while halted/idle.
    static void start(std::chrono::milliseconds timeout) {
        std::uint32_t wdv = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(timeout.count()) * slow_hz) / 1000u);
        if (wdv == 0u) {
            wdv = 1u;
        } else if (wdv > max_wdv) {
            wdv = max_wdv;
        }
        r().MR = (wdv << IP::wdv.pos) | IP::wdrsten.mask | (max_wdv << IP::wdd.pos) |
                 IP::wddbghlt.mask | IP::wdidlehlt.mask;
        feed();
    }

    // Restart the countdown (the periodic kick).
    static void feed() {
        r().CR = (0xA5u << IP::key.pos) | IP::wdrstt.mask;
    }
};

}  // namespace alloy::hal
