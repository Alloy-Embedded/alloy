/**
 * @file simple_gpio_button.cpp
 * @brief GPIO Simple API Example - Button Input
 *
 * Demonstrates input handling with the Simple tier API.
 * Shows active-low button with pull-up resistor.
 *
 * Hardware:
 * - LED on PA5/PB7 (board-specific)
 * - Button on PC13 (USER button on most Nucleo boards)
 *
 * Expected behavior:
 * - LED turns on when button is pressed
 * - LED turns off when button is released
 *
 * @note Part of MicroCore API Tier Examples
 */

#include "board.hpp"
#include "hal/api/gpio_simple.hpp"

using namespace ucore::hal;
using namespace board::pins;

int main() {
    // Simple API: Output pin (active-high LED)
    auto led = Gpio::output<led_green>();

    // Simple API: Input pin with pull-up (active-low button)
    // Button press pulls pin LOW, so we use active-low
    auto button = Gpio::input_pullup<button_user>();

    led.off();  // Start with LED off

    while (true) {
        // is_on() returns true when button is pressed (LOW)
        // because we configured it as active-low
        auto pressed = button.is_on();
        if (pressed.is_ok() && pressed.unwrap()) {
            led.on();   // Button pressed - turn LED on
        } else {
            led.off();  // Button released - turn LED off
        }

        // Small delay for debouncing
        for (volatile uint32_t i = 0; i < 1000; i++) {
            __asm volatile("nop");
        }
    }

    return 0;
}
