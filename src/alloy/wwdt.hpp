// User-facing WINDOW watchdog — the other watchdog, and a different promise.
//
//   board::window_watchdog.start();            // the window the board declares
//   for (;;) { control_step(); board::window_watchdog.feed(); }
//
// `alloy::wdt::watchdog` (the IWDG) says: reset the part if nobody feeds me
// within T. This one says: reset the part if nobody feeds me within T **or if
// somebody feeds me sooner than E**. That upper bound is the entire point.
// A loop that has lost its timebase and is spinning ten times too fast still
// feeds an independent watchdog perfectly happily; a window watchdog resets
// it. For a product with a control loop that must run AT a rate rather than
// AT LEAST a rate, that is the difference between a safety claim and a
// comfortable feeling.
//
// WHY THIS IS A SECOND ROLE AND NOT A MODE OF THE FIRST — the decision this
// file records, because the next peripheral will face the same question:
//
//  - It is not a PERSONALITY. A personality is one block with mutually
//    exclusive modes (a timer is a PWM generator or an encoder counter). The
//    IWDG and the WWDG are two blocks at two addresses on two different
//    clocks, and they run at the SAME TIME — which is the configuration a
//    safety product actually wants: the IWDG as the coarse "the core is
//    alive" backstop, the WWDG as the tight "the loop runs at its rate"
//    monitor. Modelling them as one block's two modes would make the useful
//    case unrepresentable.
//  - It is not an OPTION on `watchdog`. An option would change the contract
//    of an unchanged type: `alloy::wdt::watchdog<Inst>::feed()` is documented
//    to be safe whenever you call it, and any `libs/` driver that feeds
//    opportunistically is written against that. Turning on a window behind
//    that same facade turns those call sites into resets. The chip database
//    enforces the split at its own gate — st/wwdg_v2's class is
//    `window_watchdog`, not `watchdog` — so binding a WWDG to the `watchdog`
//    role is a named error rather than a working build with a different
//    meaning.
//
// The cost of the split is that a board with both writes two roles and a
// program that wants both feeds two dogs. That is the honest shape: they are
// two independent safety properties and neither implies the other.

#pragma once

#include <chrono>
#include <cstdint>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/types.hpp"
// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for a chip that actually has a window watchdog.
#include "alloy/hal/window_watchdog/wwdt_impl.hpp"

namespace alloy::wwdt {

// LAYER 1. Two fields, and the intersection argument is short because a
// windowed watchdog that cannot express both bounds is not a windowed
// watchdog — it is the peripheral alloy::wdt already drives.
struct config {
    //: Feed later than this after the previous feed and the part resets.
    std::chrono::microseconds deadline{};
    //: Feed SOONER than this after the previous feed and the part also
    //: resets. Zero means no early bound, which is a legal (and useless)
    //: configuration this facade will happily program: the window watchdog
    //: with its window open is an independent watchdog with a worse clock.
    std::chrono::microseconds earliest{};
};

// What the silicon was actually able to enforce. A 7-bit counter behind a
// power-of-two prescaler does not land on arbitrary microseconds, and this is
// the honest report of where it landed. Both numbers are safe to schedule
// against directly: a feed at or after `earliest` and by `deadline` is
// accepted. See st_wwdg_detail.hpp for the rounding rule and why it points
// the way it does.
struct window {
    std::chrono::microseconds earliest{};
    std::chrono::microseconds deadline{};
};

namespace detail {
// Layer 1's VALUE admission, same two mechanisms as alloy::uart's baud and
// spelled at the call site for the same reason (alloy/core/admit.hpp). Three
// impossibilities, and the third is the interesting one: the longest window a
// WWDG can enforce is a function of the BOARD's PCLK, so `4s` on a 64 MHz G0
// is a compile error that names the peripheral instead of a silent clamp to
// 524 ms that nobody notices until the field.
inline void admit_window(std::uint32_t deadline_us, std::uint32_t earliest_us,
                         std::uint32_t longest_us) {
    const bool ok = deadline_us != 0u && longest_us != 0u &&
                    earliest_us < deadline_us && deadline_us <= longest_us;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::wwdt_window();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

constexpr std::uint32_t kernel_hz_of(alloy::clock_node node, std::uint32_t ahb,
                                     std::uint32_t apb, std::uint32_t apb2,
                                     std::uint32_t sysclk) {
    switch (node) {
        case alloy::clock_node::ahb: return ahb;
        case alloy::clock_node::apb: return apb;
        case alloy::clock_node::apb2: return apb2;
        case alloy::clock_node::sysclk: return sysclk;
    }
    return sysclk;
}
}  // namespace detail

// `DefaultDeadlineUs`/`DefaultEarliestUs` come from the board file's role, so
// `start()` with no argument programs the window the BOARD declared. That is
// a small departure from `alloy::wdt::watchdog`, whose board role carries a
// `timeout_ms` that nothing reads — a project field that reaches no code is a
// comment with a schema.
template <class Inst, class Clock, std::uint32_t DefaultDeadlineUs = 0u,
          std::uint32_t DefaultEarliestUs = 0u>
class window_watchdog {
    using impl = hal::wwdt_impl<Inst>;

public:
    static constexpr std::uint32_t kernel_hz =
        detail::kernel_hz_of(Inst::kernel, Clock::ahb_hz, Clock::apb_hz, Clock::apb2_hz,
                             Clock::sysclk_hz);

    //: The bounds of what this block can enforce ON THIS BOARD's clock tree.
    //: Both are compile-time constants; a program may branch on them.
    static constexpr std::chrono::microseconds longest{impl::longest_us(kernel_hz)};
    static constexpr std::chrono::microseconds shortest{impl::shortest_us(kernel_hz)};

    // Program the window and ARM. There is no stop(): the WWDG's activation
    // bit is cleared only by a reset, on every STM32 that has one, so the
    // facade offers no method that would have to lie about switching it off.
    //
    // SHARED, with the window as the witness, for the reason wdt.hpp gives at
    // length: two call sites asking for the same window are not in conflict,
    // while `start({8ms, 4ms})` in main() and `start({100ms, 0us})` in a
    // service are — and the second silently replacing the first is how a
    // program ends up believing in a deadline nothing enforces. The witness
    // packs both bounds so the two halves of a window cannot disagree
    // unnoticed.
    window start(config c = {}) const {
        if (c.deadline.count() == 0 && c.earliest.count() == 0) {
            c.deadline = std::chrono::microseconds{DefaultDeadlineUs};
            c.earliest = std::chrono::microseconds{DefaultEarliestUs};
        }
        const auto deadline_us = static_cast<std::uint32_t>(c.deadline.count());
        const auto earliest_us = static_cast<std::uint32_t>(c.earliest.count());
        detail::admit_window(deadline_us, earliest_us,
                             static_cast<std::uint32_t>(longest.count()));
        alloy::claim::shared<Inst, alloy::claim::personality::wwdt>(
            (deadline_us << 8u) ^ earliest_us);
        const auto p = impl::start(kernel_hz, deadline_us, earliest_us);
        return window{std::chrono::microseconds{p.earliest_us},
                      std::chrono::microseconds{p.deadline_us}};
    }

    // The kick. Claims nothing — the hot path stays a single store.
    void feed() const { impl::feed(); }

    // Last gasp: the early-wakeup interrupt fires one tick before the reset,
    // on a machine that is still running. It pairs with alloy::fault — the
    // crash record is written by a handler under the same rule this one is
    // (touch nothing that could itself fail) and read by the NEXT boot, which
    // is exactly what a watchdog reset leaves behind. Install it BEFORE
    // start(): CFR.EWI is cleared only by a reset, like the activation bit.
    void on_early_wakeup(void (*fn)()) const { impl::on_early_wakeup(fn); }

    //: What start() WOULD program, without programming it — for a test, or
    //: for a program that wants to report its own margins at boot.
    static constexpr window preview(config c) {
        const auto p = impl::plan(kernel_hz, static_cast<std::uint32_t>(c.deadline.count()),
                                  static_cast<std::uint32_t>(c.earliest.count()));
        return window{std::chrono::microseconds{p.earliest_us},
                      std::chrono::microseconds{p.deadline_us}};
    }
};

// Stand-in for a board with no window watchdog role — same shape, so a
// program that feeds one compiles everywhere (guard #6). It reports a zero
// window rather than a fake one: zero means absent here as everywhere else,
// and a program that branches on `board::caps::window_watchdog` reads the
// same fact from the cap.
struct null_window_watchdog {
    static constexpr std::chrono::microseconds longest{0};
    static constexpr std::chrono::microseconds shortest{0};
    window start(config = {}) const { return {}; }
    void feed() const {}
    void on_early_wakeup(void (*)()) const {}
    static constexpr window preview(config) { return {}; }
};

}  // namespace alloy::wwdt
