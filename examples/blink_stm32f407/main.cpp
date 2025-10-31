/// Blink LED Example for STM32F407VG Discovery
///
/// This example demonstrates:
/// - Clock configuration (HSE + PLL → 168MHz)
/// - GPIO configuration and control (MODER-based)
/// - Basic LED blinking
///
/// Hardware: STM32F407VG Discovery board
/// LEDs: PD12 (Green), PD13 (Orange), PD14 (Red), PD15 (Blue)
/// This example blinks the green LED (PD12)

#include "boards/stm32f407vg/board.hpp"

int main() {
    // Initialize board (configures clock to 168MHz and enables FPU)
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
