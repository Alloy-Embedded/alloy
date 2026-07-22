// User-facing PWM: one bound channel with normalized 16-bit duty.
//
//   auto led = board::led_pwm::open({.freq_hz = 1'000});
//   led.set_duty(0x4000);   // 25%
//
// The channel is named by the pin's route signal (ch1..ch4 on ST timers;
// funcsel-style targets encode the slice/channel in the instance+pin).

#pragma once

#include <cstdint>

#include "alloy/core/routes.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"
#include "alloy/hal/pwm/pwm_impl.hpp"

namespace alloy::pwm {

struct config {
    std::uint32_t freq_hz = 1'000;
};

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

private:
    template <class, unsigned, class, alloy::signal, class>
    friend struct bind;
    handle() = default;
};

template <class Inst, unsigned Channel, class Pin, alloy::signal Sig, class Clock>
struct bind {
    static_assert(routes::routable<Pin, Inst, Sig>,
                  "pin has no route to this PWM channel on the selected chip "
                  "(check the chip's route table in alloy-devices)");

    static constexpr std::uint32_t kernel_hz() {
        switch (Inst::kernel) {
            case clock_node::ahb: return Clock::ahb_hz;
            case clock_node::apb: return Clock::apb_hz;
            case clock_node::sysclk: return Clock::sysclk_hz;
        }
        return Clock::sysclk_hz;
    }

    static handle<Inst, Channel> open(config c = {}) {
        using pin_route = routes::route<Pin, Inst, Sig>;
        hal::pin_impl<Pin>::make_af(routes::mux_value<pin_route>());
        hal::pwm_impl<Inst>::enable(kernel_hz(), c.freq_hz, Channel);
        return handle<Inst, Channel>{};
    }
};

}  // namespace alloy::pwm
