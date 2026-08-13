// The STM32 WWDG's arithmetic, with no register in sight so a host test can
// run it. RM0444 'System window watchdog (WWDG)' states one formula:
//
//     t_WWDG = t_PCLK * 4096 * 2^WDGTB * (T[5:0] + 1)
//
// and two rules that are not in it. T6 is not part of the count — the reset
// fires the instant it clears (0x40 -> 0x3F) — so a reload is 0x40..0x7F and
// only the low six bits are time. And a refresh (any write to CR) while
// T[6:0] is still ABOVE CFR.W resets the part immediately, which is the early
// bound this whole peripheral exists for.
//
// THE ROUNDING RULE, because a 7-bit counter cannot land on an arbitrary pair
// of microseconds and the choice is a safety decision, not a taste one:
//
//     the programmed window always CONTAINS the requested one.
//
// The deadline rounds UP (the dog never bites before the user's own deadline)
// and the early bound rounds DOWN (a feed the user believed was legal is never
// rejected). Both errors are therefore toward accepting a compliant feeder;
// neither can cause a spurious reset, and both are bounded by one tick.
//
// The REPORTED pair rounds the other way — deadline down, earliest up — so the
// two numbers handed back are directly usable as a schedule: feed at or after
// `earliest_us` and by `deadline_us` and the silicon accepts it. The two rules
// together give request ⊆ reported ⊆ enforced, which is what
// tests/test_wwdt.cpp sweeps.

#pragma once

#include <cstdint>

namespace alloy::hal::detail {

// Fixed by the IP, not by the version: every WWDG divides PCLK by 4096 before
// the WDGTB prescaler, counts seven bits, and arms with bit 6.
inline constexpr std::uint32_t wwdg_prediv = 4096u;
inline constexpr std::uint32_t wwdg_arm_bit = 0x40u;
inline constexpr std::uint32_t wwdg_max_counts = 64u;  // T[5:0] + 1
inline constexpr std::uint32_t wwdg_cr_wdga = 0x80u;

struct wwdg_program {
    std::uint8_t wdgtb = 0u;
    std::uint8_t reload = 0x7Fu;  // CR.T, arm bit included
    std::uint8_t win = 0x7Fu;     // CFR.W
    //: A feed at or after this is accepted; a feed by this is accepted.
    std::uint32_t earliest_us = 0u;
    std::uint32_t deadline_us = 0u;
};

//: Microseconds per counter tick, times 1e6 — kept as a numerator so the
//: whole calculation stays in integers.
constexpr std::uint64_t wwdg_tick_scaled(std::uint32_t wdgtb) {
    return static_cast<std::uint64_t>(wwdg_prediv << wdgtb) * 1'000'000ull;
}

//: The longest deadline this block can enforce on this clock. It is a
//: function of the BOARD's PCLK, which is why asking for more is a compile
//: error at the facade rather than a silent clamp here: at 64 MHz the WWDG
//: tops out around half a second, three orders of magnitude short of the
//: IWDG's ~32 s, and a user who does not know that will write 4s and believe
//: it.
constexpr std::uint32_t wwdg_longest_us(std::uint32_t pclk_hz, std::uint32_t timebase_max) {
    if (pclk_hz == 0u) {
        return 0u;
    }
    return static_cast<std::uint32_t>(
        (wwdg_max_counts * wwdg_tick_scaled(timebase_max)) / pclk_hz);
}

//: The shortest deadline it can enforce: one tick at the finest prescaler.
constexpr std::uint32_t wwdg_shortest_us(std::uint32_t pclk_hz) {
    if (pclk_hz == 0u) {
        return 0u;
    }
    return static_cast<std::uint32_t>(wwdg_tick_scaled(0u) / pclk_hz);
}

//: Pick WDGTB, CR.T and CFR.W for a requested window. `timebase_max` is the
//: instance's own generated degree (Inst::feat::timebase_max), not a literal:
//: wwdg_v1's WDGTB is two bits wide and this same code has to be right there.
constexpr wwdg_program wwdg_plan(std::uint32_t pclk_hz, std::uint32_t timebase_max,
                                 std::uint32_t deadline_us, std::uint32_t earliest_us) {
    wwdg_program p{};
    if (pclk_hz == 0u) {
        return p;
    }
    // Never let a runtime value overflow the arithmetic below. The facade's
    // admit check has already refused this case at compile time wherever the
    // value was a constant; this is the same bound restated where the
    // multiplication happens.
    const std::uint32_t longest = wwdg_longest_us(pclk_hz, timebase_max);
    if (deadline_us > longest) {
        deadline_us = longest;
    }
    if (earliest_us > deadline_us) {
        earliest_us = deadline_us;
    }

    std::uint32_t tb = 0u;
    std::uint64_t counts = 1u;
    for (;; ++tb) {
        const std::uint64_t scale = wwdg_tick_scaled(tb);
        // ceil: the enforced deadline is never shorter than the asked one.
        counts = (static_cast<std::uint64_t>(deadline_us) * pclk_hz + scale - 1u) / scale;
        if (counts == 0u) {
            counts = 1u;
        }
        if (counts <= wwdg_max_counts || tb >= timebase_max) {
            break;
        }
    }
    if (counts > wwdg_max_counts) {
        counts = wwdg_max_counts;
    }
    const std::uint64_t scale = wwdg_tick_scaled(tb);

    // floor: the enforced early bound is never later than the asked one, so a
    // feed the caller thought was legal is never the one that resets the part.
    std::uint64_t early = (static_cast<std::uint64_t>(earliest_us) * pclk_hz) / scale;
    if (early > counts - 1u) {
        early = counts - 1u;  // leave at least one tick in which a feed is legal
    }

    p.wdgtb = static_cast<std::uint8_t>(tb);
    p.reload = static_cast<std::uint8_t>(wwdg_arm_bit | (counts - 1u));
    p.win = static_cast<std::uint8_t>(p.reload - early);
    // Reported the safe way round — see the header comment.
    p.deadline_us = static_cast<std::uint32_t>((counts * scale) / pclk_hz);
    p.earliest_us = static_cast<std::uint32_t>((early * scale + pclk_hz - 1u) / pclk_hz);
    return p;
}

}  // namespace alloy::hal::detail
