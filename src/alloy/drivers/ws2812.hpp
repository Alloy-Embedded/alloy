// Bit-banged WS2812 (NeoPixel) driver for a single RGB LED used as a board
// LED role (Waveshare RP2040-Zero and friends).
//
// Timing is derived from the board clock at compile time — WS2812 tolerates
// about ±150 ns, so a working LED doubles as a clock-accuracy check. The
// 24-bit frame (~30 µs) runs with IRQs masked so the 1 kHz SysTick cannot
// stretch a bit. Exposes the same on/off/toggle/set_high/set_low surface as
// alloy::gpio::output so board::led stays role-compatible.

#pragma once

#include <cstdint>

#include "alloy/arch/irq.hpp"
#include "alloy/hal/gpio/pin_impl.hpp"

namespace alloy::drivers {

template <class Pin, class Clock>
class ws2812 {
    using impl = hal::pin_impl<Pin>;

    // ~3 cycles per iteration on Cortex-M0+ (subs + branch taken). GCC drops
    // inline asm into divided syntax on ARM; declare unified explicitly.
    static void delay_cycles(std::uint32_t n) {
        __asm volatile(
            ".syntax unified\n\t"
            "1: subs %0, #3\n\t"
            "bhi 1b"
            : "+l"(n)::"cc");
    }

    static constexpr std::uint32_t ns_to_cycles(std::uint32_t ns) {
        return static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(Clock::sysclk_hz) * ns) / 1'000'000'000u);
    }

    static void send_grb(std::uint32_t grb) {
        const arch::irq_state saved = arch::irq_save();
        for (std::uint32_t mask = 1u << 23; mask != 0u; mask >>= 1) {
            if (grb & mask) {
                impl::set_high();
                delay_cycles(ns_to_cycles(800u));
                impl::set_low();
                delay_cycles(ns_to_cycles(450u));
            } else {
                impl::set_high();
                delay_cycles(ns_to_cycles(400u));
                impl::set_low();
                delay_cycles(ns_to_cycles(850u));
            }
        }
        arch::irq_restore(saved);
    }

public:
    static void init() {
        impl::make_output();
        impl::set_low();
    }

    void on() const {
        send_grb(0x200000u);  // dim green (GRB order)
        state() = true;
    }
    void off() const {
        send_grb(0u);
        state() = false;
    }
    void toggle() const {
        if (state()) {
            off();
        } else {
            on();
        }
    }

    // OutputPin-compatible aliases.
    void set_high() const { on(); }
    void set_low() const { off(); }

private:
    static bool& state() {
        static bool s = false;
        return s;
    }
};

}  // namespace alloy::drivers
