// The time base shared by every ST timer alloy curates — the CR1/DIER/SR/EGR/
// CNT/PSC/ARR core, and nothing else.
//
// ONE IMPLEMENTATION, FOUR IP VERSIONS, and that is a claim about the silicon
// rather than a convenience. Upstream's own register model factors these seven
// registers out as a virtual block (`TIM_CORE`) that TIM_BASIC, TIM_1CH,
// TIM_1CH_CMP and TIM_2CH_CMP all extend at identical offsets; the four
// curated IP files in alloy-devices reproduce that, offsetof-asserted per chip.
// So the four thin headers beside this one are the whole per-IP driver: each
// binds `tick_impl<Inst>` for its own IP tag, and they differ in nothing.
//
// WHAT IS DELIBERATELY NOT HERE: channels. Three of the four blocks have one
// or two capture/compare units and two of those gate their outputs behind
// BDTR.MOE — a PWM driver for them is a separate driver, and writing it as a
// flag on this one is exactly the mistake that made `st/tim_gp16` look like a
// candidate for TIM6 in the first place.
//
// BEHAVIOR only: bases, gates and field positions come from generated headers.
// NOT WITNESSED ON SILICON — no board was attached to any of this, and
// Renode 1.16.1's Timers.STM32_Timer does not model these instances on the G0.

#pragma once

#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/tick/st_tim_divisor.hpp"

namespace alloy::hal::detail {

template <class Inst>
struct st_tick_timebase {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    //: The pair actually programmed, kept so achieved_hz() reports the rate
    //: the block runs at rather than the one that was asked for.
    static inline tick_divisor programmed{0u, 0u};

    // THE FACADE ASKS THE DRIVER, not the arithmetic. `alloy::tick` is
    // portable and this file is not, so the admission test and the achieved
    // rate are members here rather than free functions the facade names —
    // which is also what makes them DEPENDENT names, the difference between a
    // header that compiles on a board with no ST timer and one that does not.
    [[nodiscard]] static constexpr bool representable(std::uint32_t kernel_hz,
                                                      std::uint32_t hz) {
        return tick_representable(kernel_hz, hz);
    }

    [[nodiscard]] static std::uint32_t achieved_hz(std::uint32_t kernel_hz) {
        return tick_achieved_hz(kernel_hz, programmed);
    }

    [[nodiscard]] static std::uint32_t period_ticks() {
        return static_cast<std::uint32_t>(programmed.arr) + 1u;
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t hz) {
        alloy::gate_on(Inst::gate);
        programmed = tick_pick(kernel_hz, hz);
        IP::psc.write(r(), programmed.psc);
        IP::arr.write(r(), programmed.arr);
        // URS: raise the update event ONLY on overflow. Without it the UG
        // write below — and every later reconfiguration — sets UIF too, so the
        // first expired() would return true immediately and the caller's first
        // period would be zero long.
        IP::urs.set(r());
        IP::arpe.set(r());
        r().EGR = IP::ug.mask;  // latch PSC and ARR now, not one period late
        IP::cen.set(r());
    }

    static void stop() { IP::cen.clear(r()); }
    static void start() { IP::cen.set(r()); }

    [[nodiscard]] static std::uint16_t count() {
        return static_cast<std::uint16_t>(IP::cnt.read(r()));
    }

    static void reset_count() { IP::cnt.write(r(), 0u); }

    // True once per period, and CONSUMING: the flag is cleared here, so a
    // caller polling in a loop counts periods rather than seeing one forever.
    [[nodiscard]] static bool take_expired() {
        if (IP::uif.read(r()) == 0u) {
            return false;
        }
        // UIF is rc_w0 — writing ONE to a status bit leaves it alone, so the
        // clear is a write of all ones with this bit zeroed. A plain
        // read-modify-write would clear every other flag in the register on
        // the blocks that have others.
        r().SR = static_cast<std::uint32_t>(~IP::uif.mask);
        return true;
    }

    static void irq_on_update(bool on) {
        if (on) {
            IP::uie.set(r());
        } else {
            IP::uie.clear(r());
        }
    }

    static void dma_on_update(bool on) {
        if (on) {
            IP::ude.set(r());
        } else {
            IP::ude.clear(r());
        }
    }

    // Master mode: emit the update event on TRGO, which is what an ADC or a
    // DAC waits on. Only instantiated where the IP HAS an MMS field — see the
    // `feat::trgo` guard in alloy/tick.hpp — because on TIM14/TIM16/TIM17 this
    // register bit does not exist and `IP::mms` is not a member to name.
    static void trigger_on_update()
        requires requires { IP::mms; } {
        // The encoding comes from the register's generated flags enum, which
        // carries it already shifted into place — the driver never spells the
        // number 2, and a data correction reaches it without an edit here.
        const std::uint32_t cur = static_cast<std::uint32_t>(r().CR2);
        r().CR2 = (cur & ~IP::mms.mask) |
                  static_cast<std::uint32_t>(IP::cr2::mms_update);
    }
};

}  // namespace alloy::hal::detail
