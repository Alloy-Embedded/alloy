/**
 * @file main.cpp
 * @brief GPIO template test for SAME70
 *
 * Tests the template-based GPIO implementation by:
 * - Configuring LED pins as outputs
 * - Toggling LEDs
 * - Reading button state
 *
 * Board: SAME70 Xplained Ultra
 * - LED0 (Green): PC8
 * - LED1 (Blue): PC9
 * - Button SW0: PA11
 */

#include "hal/platform/same70/gpio.hpp"
#include "boards/atmel_same70_xpld/board.hpp"

using namespace alloy::hal::same70;

int main() {
    // Initialize board (clock, etc.)
    Board::initialize();

    // Create GPIO instances using type aliases
    auto led_green = Led0{};
    auto led_blue = Led1{};
    auto button = Button0{};

    // Configure LEDs as outputs
    led_green.setMode(GpioMode::Output);
    led_blue.setMode(GpioMode::Output);

    // Configure button as input with pull-up
    button.setMode(GpioMode::Input);
    button.setPull(GpioPull::Up);
    button.enableFilter();  // Enable glitch filter

    // Turn off LEDs initially
    led_green.clear();
    led_blue.clear();

    // Main loop
    uint32_t counter = 0;
    while (true) {
        // Toggle green LED every loop iteration
        led_green.toggle();

        // Blue LED controlled by button
        // Button is active-low (pressed = LOW)
        auto button_state = button.read();
        if (button_state.is_ok()) {
            if (!button_state.value()) {
                // Button pressed - turn on blue LED
                led_blue.set();
            } else {
                // Button released - turn off blue LED
                led_blue.clear();
            }
        }

        // Simple delay
        Board::delay_ms(500);

        counter++;
    }

    return 0;
}
