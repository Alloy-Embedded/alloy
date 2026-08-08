// The RL78 half of alloy::arch — interrupt masking and line control.
//
// Two things here are the opposite of the Cortex-M backend beside it, and both
// are easy to get backwards:
//   * MK is a MASK. Enabling a line CLEARS its bit; the NVIC's ISER sets one.
//   * PSW.IE is a single global enable. There is no BASEPRI, so a priority-
//     bounded critical section degrades to a full mask, exactly as it does on
//     ARMv6-M and Xtensa.

#include <cstdint>

#include "alloy/arch/irq.hpp"
#include "alloy/arch/rl78/intc.hpp"
#include "alloy/arch/rl78/systick.hpp"

namespace {

//: PSW lives in the SFR area; bit 7 is IE. Read it rather than tracking a
//: shadow, so an irq_save inside an ISR restores what was actually there.
constexpr std::uintptr_t kPsw = alloy::arch::rl78::near(0xFFFFA);
constexpr std::uint8_t kIe = 0x80;

[[nodiscard]] inline std::uint8_t psw() {
    return *reinterpret_cast<volatile std::uint8_t*>(kPsw);
}

}  // namespace

namespace alloy::arch {

irq_state irq_save() {
    // Read first, then mask: the returned state must describe the world before
    // the DI, and "memory" keeps shared stores from crossing the boundary.
    const std::uint8_t before = psw();
    __asm volatile("di" ::: "memory");
    return before;
}

void irq_restore(irq_state state) {
    if ((static_cast<std::uint8_t>(state) & kIe) != 0u) {
        __asm volatile("ei" ::: "memory");
    } else {
        __asm volatile("di" ::: "memory");
    }
}

void irq_line_enable(unsigned n) {
    if (n >= rl78::kMaxVector) {
        return;
    }
    using rl78::plane;
    // Clear the pending request first: a source that latched while masked
    // would otherwise fire the instant it is unmasked, which on a re-enable is
    // a stale event rather than a new one.
    rl78::reg(plane::request, n) =
        static_cast<std::uint16_t>(rl78::reg(plane::request, n) & ~rl78::bit_of(n));
    rl78::reg(plane::mask, n) =
        static_cast<std::uint16_t>(rl78::reg(plane::mask, n) & ~rl78::bit_of(n));
}

void irq_line_disable(unsigned n) {
    if (n >= rl78::kMaxVector) {
        return;
    }
    rl78::reg(rl78::plane::mask, n) =
        static_cast<std::uint16_t>(rl78::reg(rl78::plane::mask, n) | rl78::bit_of(n));
}

void irq_line_priority(unsigned n, std::uint8_t level) {
    if (n >= rl78::kMaxVector) {
        return;
    }
    using rl78::plane;
    // Two bits, (PR1,PR0), 0 most urgent. alloy's contract is "0 == most
    // urgent, higher == less", so anything above 3 saturates at the least
    // urgent level rather than wrapping into a MORE urgent one.
    const std::uint8_t lvl = level > 3u ? 3u : level;
    const std::uint16_t bit = rl78::bit_of(n);
    auto put = [&](plane p, bool on) {
        volatile std::uint16_t& r = rl78::reg(p, n);
        r = static_cast<std::uint16_t>(on ? (r | bit) : (r & ~bit));
    };
    put(plane::prio_low, (lvl & 0x1u) != 0u);
    put(plane::prio_high, (lvl & 0x2u) != 0u);
}

irq_state irq_save_below(std::uint8_t /*level*/) {
    // No priority mask on this architecture — see the header's note.
    return irq_save();
}

void irq_restore_below(irq_state state) { irq_restore(state); }

}  // namespace alloy::arch
