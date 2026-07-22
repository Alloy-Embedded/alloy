// Portable PWM fade — the LED breathes instead of blinking. On boards
// without a PWM-capable LED the stub keeps this compiling and the plain
// LED blinks as a fallback. Zero #ifdefs.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

int main() {
    board::init();
    auto led = board::led_pwm::open({.freq_hz = 1'000});

    while (true) {
        if constexpr (board::caps::led_pwm) {
            for (std::uint32_t d = 0; d <= 65535u; d += 1310u) {
                led.set_duty(static_cast<std::uint16_t>(d));
                alloy::sleep_for(20ms);
            }
            for (std::uint32_t d = 65535u; d >= 1310u; d -= 1310u) {
                led.set_duty(static_cast<std::uint16_t>(d));
                alloy::sleep_for(20ms);
            }
        } else {
            board::led.toggle();
            alloy::sleep_for(500ms);
        }
    }
}
