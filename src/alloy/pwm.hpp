// User-facing PWM: one bound channel with normalized 16-bit duty.
//
//   auto led = board::led_pwm::open({.freq_hz = 1'000});
//   led.set_duty(0x4000);   // 25%
//
// The channel is named by the pin's route signal (ch1..ch4 on ST timers;
// funcsel-style targets encode the slice/channel in the instance+pin).

#pragma once

#include <cstdint>
#include <span>

#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/pwm/pwm_impl.hpp"

namespace alloy::pwm {

// BLOCK-SCOPED, and that is the whole subtlety of this facade. A timer has one
// prescaler and one auto-reload for all four of its channels, so `freq_hz` is
// not a property of the channel you are opening — it is a property of the
// timer, stated at a channel's call site because that is the only call there
// is. Two channels of one timer asking for different frequencies is a
// contradiction the block cannot honour; the claim below refuses it instead of
// letting the second open() silently win.
//: Layer-1 vocabulary, declared in hal/pwm/pwm_impl.hpp so a driver can take
//: it, aliased here so a caller never spells `hal`. Same arrangement as
//: alloy::bridge::config.
using alignment = hal::pwm_alignment;
using trigger = hal::pwm_trigger;
using config = hal::pwm_config;

namespace detail {
// Layer 1's VALUE admission — see alloy/core/admit.hpp. The driver computes
// `period_ticks = tick_hz / freq_hz`, so zero was a fold-to-unreachable that
// landed in the double-open trap.
inline void admit_freq(std::uint32_t freq_hz, std::uint32_t kernel) {
    const bool ok = freq_hz != 0u && kernel != 0u && freq_hz <= kernel;
    if (__builtin_constant_p(ok) && !ok) {
        alloy::core::admit::pwm_freq();
    }
    if (!ok) {
        alloy::trap<alloy::trap_code::impossible_config>();
    }
}

// ONE FACT, STATED TWICE, so make the two spellings agree at compile time.
// `bind` carries the channel ORDINAL (which the driver programs, and which is
// the key of the sub-resource claim) and the route SIGNAL (which picks the pin
// mux). Nothing tied them together: `bind<tim2_t, 2u, pa5_t, signal::ch1, …>`
// muxed PA5 onto CH1 while programming CH2 and claiming ownership of CH2, so
// two binds that genuinely contest CH1 could both be admitted. The generator
// always emits them consistently; a hand-written bind had no such promise.
//
// A signal that is NOT a numbered channel is left alone on purpose:
// funcsel-style backends name the slice through the instance and the pin
// rather than the signal, so there is no ordinal in it to disagree with.
//: Does this instance's curated data say it has a trigger output? Absent
//: `feat` reads as "no" rather than as a substitution failure, so a timer IP
//: nobody has given a feat block yet refuses the request with a message
//: instead of a compiler error about a missing member.
template <class Inst>
inline constexpr bool has_trigger_output = [] {
    if constexpr (requires { Inst::feat::trgo; }) {
        return Inst::feat::trgo != 0u;
    } else {
        return false;
    }
}();

//: A trigger asked of a block that has none would be accepted and then never
//: fire — the silent failure this refuses. Same shape as admit_freq: an
//: [[gnu::error]] where the optimiser can prove it, and an unconditional trap
//: where it cannot (measured, that is most of -O0/-Og — see the note in
//: alloy/bridge.hpp about when the attribute actually fires).
template <class Inst>
inline void admit_trigger(pwm::trigger t) {
    if constexpr (!has_trigger_output<Inst>) {
        const bool asked = t != pwm::trigger::none;
        if (__builtin_constant_p(asked) && asked) {
            alloy::core::admit::pwm_no_trigger();
        }
        if (asked) {
            alloy::trap<alloy::trap_code::impossible_config>();
        }
    } else {
        (void)t;
    }
}

constexpr bool channel_matches_signal(unsigned channel, alloy::signal sig) {
    switch (sig) {
        case alloy::signal::ch1: return channel == 1u;
        case alloy::signal::ch2: return channel == 2u;
        case alloy::signal::ch3: return channel == 3u;
        case alloy::signal::ch4: return channel == 4u;
        default: return true;
    }
}
}  // namespace detail

template <class Inst, unsigned Channel>
class handle {
public:
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&&) noexcept = default;
    handle& operator=(handle&&) noexcept = default;

    // duty: 0 = off .. 0xFFFF = always on.
    void set_duty(std::uint16_t duty) const {
        hal::pwm_impl<Inst>::set_duty(Channel, duty);
    }
    void off() const { hal::pwm_impl<Inst>::set_duty(Channel, 0u); }

    // Stream RAW compare values (0..period_ticks()-1) from memory to this
    // channel's duty register on every timer UPDATE, circularly, with zero
    // CPU. The waveform must OUTLIVE the stream (static buffer).
    template <class Chan>
    void stream_duty(const Chan& dma, std::span<const std::uint16_t> waveform) const
        requires requires {
            hal::pwm_impl<Inst>::dma_update_begin();
            Inst::dmareq_up;
        }
    {
        // Arm the channel first, then open the request gate — an update
        // event in between would otherwise issue a request nobody serves.
        dma.start_m2p_circular_u16(waveform, hal::pwm_impl<Inst>::duty_addr(Channel),
                                   Inst::dmareq_up);
        hal::pwm_impl<Inst>::dma_update_begin();
    }

    template <class Chan>
    void stop_stream(const Chan& dma) const
        requires requires { hal::pwm_impl<Inst>::dma_update_end(); }
    {
        dma.stop();
        hal::pwm_impl<Inst>::dma_update_end();
    }

    [[nodiscard]] std::uint32_t period_ticks() const
        requires requires { std::uint32_t{hal::pwm_impl<Inst>::period_ticks}; }
    {
        return hal::pwm_impl<Inst>::period_ticks;
    }

private:
    template <class, unsigned, class, alloy::signal, class>
    friend struct bind;
    handle() = default;
};

template <class Inst, unsigned Channel, class Pin, alloy::signal Sig, class Clock>
struct bind {
    static_assert(detail::channel_matches_signal(Channel, Sig),
                  "PWM bind names one channel ordinal and a different channel "
                  "signal — the ordinal is what the driver programs and what "
                  "owns the channel, the signal is what routes the pin, and "
                  "they have to be the same channel");
    static_assert(routes::routable<Pin, Inst, Sig>,
                  "pin has no route to this PWM channel on the selected chip "
                  "(check the chip's route table in alloy-devices)");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::apb2: return Clock::apb2_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    static handle<Inst, Channel> open(config c = {}) {
        // SHARED, not exclusive: four channels of one timer are one legitimate
        // owner in one personality. What they may not do is disagree about the
        // block-scoped value — `enable()` writes PSC and ARR unconditionally,
        // so before this the second channel's frequency silently replaced the
        // first's and both handles kept working at the wrong rate. The witness
        // is that value; a mismatch traps with its own code.
        detail::admit_freq(c.freq_hz, kernel_hz());
        // The CHANNEL is still exclusive — sharing the block does not mean
        // opening one channel twice is suddenly fine. This claim is per
        // (INSTANCE, CHANNEL), not per binder type: `detail_channel_opened`
        // used to live here as a `static` member of `bind`, which is templated
        // on the PIN too, so `bind<tim2_t, 1, pa5_t, ...>` and
        // `bind<tim2_t, 1, pa0_t, ...>` — two of TIM2_CH1's four legal routes
        // on the G0B1RE — got one flag EACH. Both opened, both muxed their pin
        // onto the same output compare, and neither said a word. That is hole
        // (A) exactly, one scope down.
        alloy::claim::sub_exclusive<Inst, Channel, alloy::claim::personality::pwm>();
        alloy::claim::shared<Inst, alloy::claim::personality::pwm>(c.freq_hz);
        using pin_route = routes::route<Pin, Inst, Sig>;
        hal::pin_impl<Pin>::make_af(routes::mux_value<pin_route>());
        detail::admit_trigger<Inst>(c.trigger);
        hal::pwm_impl<Inst>::enable(kernel_hz(), c, Channel);
        return handle<Inst, Channel>{};
    }
};

}  // namespace alloy::pwm
