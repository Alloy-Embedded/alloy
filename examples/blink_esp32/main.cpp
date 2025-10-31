/// Blink LED Example for ESP32 DevKit
///
/// This example demonstrates:
/// - Clock configuration (XTAL + PLL → 160MHz)
/// - GPIO configuration and control
/// - Basic LED blinking
///
/// Hardware: ESP32 DevKit board
/// LED: GPIO2 (active HIGH, built-in blue LED on many boards)

#include "boards/esp32_devkit/board.hpp"

int main() {
    // Initialize board (configures clock to 160MHz)
    auto result = alloy::board::init();
    if (result.is_error()) {
        // Clock initialization failed - trap
        while (true) {
            __asm__ volatile("waiti 0");
        }
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
