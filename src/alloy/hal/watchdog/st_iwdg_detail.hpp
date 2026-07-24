// Shared STM32 IWDG logic. The v1 and v2 register blocks differ only by v2's
// WINR (window — left at its reset max, so feeding is allowed any time), so
// start()/feed() are identical; each per-version wdt_impl delegates here.
// IWDG_KR = 0xCCCC forces the ~32 kHz LSI on automatically, so no separate LSI
// enable is needed. The counter is 12-bit off a /4../256 prescaler → ~32 s max.

#pragma once

#include <chrono>
#include <cstdint>

namespace alloy::hal::detail {

// Nominal LSI. It is an imprecise RC (±several %), which is fine for a
// watchdog margin — the exact bite time is approximate by design.
inline constexpr std::uint32_t iwdg_lsi_hz = 32'000;

// Configure the IWDG for `timeout` and start it. Regs is the generated IWDG
// register struct (KR/PR/RLR/SR, plus WINR on v2 which we never touch).
template <class Regs>
inline void iwdg_configure(Regs& r, std::chrono::milliseconds timeout) {
    // Choose the finest prescaler whose 12-bit reload still spans the timeout.
    std::uint32_t pr = 0;
    std::uint32_t counts = 1;
    for (; pr <= 6u; ++pr) {
        const std::uint32_t tick_hz = iwdg_lsi_hz / (4u << pr);
        counts = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(timeout.count()) * tick_hz) / 1000u);
        if (counts <= (0xFFFu + 1u)) {
            break;
        }
    }
    if (pr > 6u) {  // longer than the counter can hold — clamp to the max
        pr = 6u;
        counts = 0xFFFu + 1u;
    }
    if (counts == 0u) {
        counts = 1u;
    }
    r.KR = 0xCCCCu;         // start (forces the LSI on)
    r.KR = 0x5555u;         // unlock PR/RLR
    r.PR = pr;
    r.RLR = counts - 1u;    // reload counts down from RLR, so RLR+1 ticks
    while ((r.SR & 0x7u) != 0u) {  // wait for PVU/RVU/WVU to settle
    }
    r.KR = 0xAAAAu;         // reload
}

// Restart the countdown.
template <class Regs>
inline void iwdg_kick(Regs& r) {
    r.KR = 0xAAAAu;
}

}  // namespace alloy::hal::detail
