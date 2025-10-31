/// Blink LED Example for Arduino Zero
///
/// This example demonstrates:
/// - Clock configuration (DFLL48M → 48MHz)
/// - GPIO configuration and control (PORT-based)
/// - Basic LED blinking
///
/// Hardware: Arduino Zero board (ATSAMD21G18)
/// LED: PA17 (Yellow LED marked "L")

#include "boards/arduino_zero/board.hpp"

int main() {
    // Initialize board (configures clock to 48MHz via DFLL48M)
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
