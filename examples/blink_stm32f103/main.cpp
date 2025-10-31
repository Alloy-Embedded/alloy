/// Blink LED Example for STM32F103C8 (Blue Pill)
///
/// This example demonstrates:
/// - Clock configuration (HSE + PLL → 72MHz)
/// - GPIO configuration and control
/// - Basic LED blinking
///
/// Hardware: STM32F103C8 Blue Pill board
/// LED: PC13 (active LOW)

#include "boards/stm32f103c8/board.hpp"

int main() {
    // Initialize board (configures clock to 72MHz and enables GPIO clocks)
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
