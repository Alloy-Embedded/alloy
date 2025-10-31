/// Blink LED Example for Raspberry Pi Pico
///
/// This example demonstrates:
/// - Clock configuration (XOSC + PLL → 125MHz)
/// - GPIO configuration and control (SIO-based)
/// - Basic LED blinking
///
/// Hardware: Raspberry Pi Pico board (RP2040)
/// LED: GPIO25 (Green onboard LED)

#include "boards/raspberry_pi_pico/board.hpp"

int main() {
    // Initialize board (configures clock to 125MHz via PLL_SYS)
    auto result = alloy::board::init();
    if (result.is_error()) {
        // Clock initialization failed - trap
        while (true);
    }

    // Initialize LED pin
    alloy::board::init_led();

    // Blink LED forever at 1Hz (500ms on, 500ms off)
    while (true) {
        alloy::board::led_on();
        alloy::board::delay_ms(500);

        alloy::board::led_off();
        alloy::board::delay_ms(500);
    }

    return 0;
}
