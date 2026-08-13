// User-facing periodic timer: a hardware counter that raises an update event
// at a requested rate.
//
//   auto t = board::tick::open({.hz = 10});
//   while (true) {
//       if (t.expired()) { led.toggle(); }
//   }
//
// NOT alloy::sleep_for(). That is the ARCHITECTURE timebase — one SysTick per
// program, started by board::init(), the thing every delay is measured
// against. This is a peripheral you open, own, and can point at something
// else: an ADC's trigger, a DMA request, an ISR at a rate the SysTick does not
// run at. A program uses both, and the names are close because the concepts
// are; the distinction is "the clock everything shares" versus "a clock you
// asked for".
//
// WHAT THIS FACADE DELIBERATELY DOES NOT DO: channels. Four of the six blocks
// it serves on the STM32G0 have a capture/compare unit, two of those gate
// their outputs behind BDTR.MOE, and one of them is a general-purpose timer
// that alloy already drives as `alloy::pwm`. Output generation is a different
// personality of the same block and gets a different facade when it is
// written — see the claim below, which is what makes "different personality"
// mean something at run time rather than in a comment.

#pragma once

#include <cstdint>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/tick/tick_impl.hpp"

namespace alloy::tick {

// LAYER 1 — portable, honoured by every driver that implements this facade.
//
// ONE FIELD, and the reason it is one is worth stating because the temptation
// to add `period_us` or `prescaler` is constant: a period is the same fact as
// a rate, and a prescaler is the same fact as both PLUS a choice the driver
// makes better (see hal/tick/st_tim_divisor.hpp — it picks the finest counter
// that reaches the rate, which no caller can do without knowing the kernel
// clock). A second spelling of one fact is a second thing that can disagree.
struct config {
    std::uint32_t hz = 1'000;  //: update events per second
};

namespace detail {

// Layer 1's VALUE admission — see alloy/core/admit.hpp. A rate the kernel
// clock cannot divide down to is a compile error at every literal call site
// and a named trap otherwise. BOTH ENDS ARE REAL and neither is exotic on a
// 64 MHz G0: 100 MHz is above the clock, and 0.9 Hz needs more than the
// 2^32 of division two 16-bit registers hold.
inline void admit_hz(std::uint32_t hz, std::uint32_t kernel_hz) {
    const bool ok = alloy::hal::tick_representable(kernel_hz, hz);
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::tick_hz();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

}  // namespace detail

template <class Inst>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    // True once per period, and it CONSUMES the event: polling in a loop
    // counts periods. A caller that misses one is late, not wrong — the flag
    // is one bit, so two elapsed periods read as one.
    [[nodiscard]] bool expired() const { return hal::tick_impl<Inst>::take_expired(); }

    // Where the counter is inside the current period, 0 .. period_ticks()-1.
    [[nodiscard]] std::uint16_t count() const { return hal::tick_impl<Inst>::count(); }

    void restart() const { hal::tick_impl<Inst>::reset_count(); }
    void stop() const { hal::tick_impl<Inst>::stop(); }
    void start() const { hal::tick_impl<Inst>::start(); }

    // The rate the block ACTUALLY runs at. Rarely the requested one: the
    // division is integer, so 64 MHz / 3 Hz is not 3 Hz. A caller that cares
    // reads this instead of assuming it got what it asked for.
    [[nodiscard]] std::uint32_t achieved_hz() const {
        return hal::tick_achieved_hz(kernel_hz_, hal::tick_impl<Inst>::programmed);
    }
    [[nodiscard]] std::uint32_t period_ticks() const {
        return static_cast<std::uint32_t>(hal::tick_impl<Inst>::programmed.arr) + 1u;
    }

    // Raise the instance's interrupt on every update. The vector is
    // `Inst::irq`; attaching a handler to it is alloy::irq's business, not
    // this facade's.
    void irq_on_update(bool on = true) const { hal::tick_impl<Inst>::irq_on_update(on); }
    void dma_on_update(bool on = true) const { hal::tick_impl<Inst>::dma_on_update(on); }

    // Drive this timer's TRGO from the update event — the standard way to
    // clock an ADC or a DAC from a timer instead of from the CPU.
    //
    // GUARDED BY A GENERATED NUMBER, not by a family name. `feat::trgo` comes
    // from the IP's curated data, and it is 0 on the STM32G0's TIM14, TIM16
    // and TIM17: those blocks either have no CR2 at all or have one whose bits
    // 6:4 are the output-idle state. Calling this on one of them is a compile
    // error naming the instance, which is the only way a caller finds out
    // before shipping — the hardware's answer is a trigger that never fires.
    void trigger_on_update() const
        requires (Inst::feat::trgo != 0u)
    {
        hal::tick_impl<Inst>::trigger_on_update();
    }

private:
    template <class, class>
    friend struct bind;
    explicit handle(std::uint32_t kernel_hz) : kernel_hz_(kernel_hz) {}
    std::uint32_t kernel_hz_;
};

template <class Inst, class Clock>
struct bind {
    using inst = Inst;

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    static handle<Inst> open(config c = {}) {
        detail::admit_hz(c.hz, kernel_hz());
        // EXCLUSIVE, unlike alloy::pwm's claim on the same kind of block. PWM
        // shares a timer because four channels of one block are four
        // legitimate owners of one personality; a time base has no channels to
        // share, so a second opener is not a co-owner asking for the same
        // rate, it is a second program overwriting PSC and ARR.
        alloy::claim::exclusive<Inst, alloy::claim::personality::tick>();
        hal::tick_impl<Inst>::enable(kernel_hz(), c.hz);
        return handle<Inst>{kernel_hz()};
    }
};

}  // namespace alloy::tick
