/**
 * @file simple_gpio_blink.cpp
 * @brief GPIO Simple API Example - LED Blink
 *
 * Demonstrates the Simple tier API from hal/api/gpio_simple.hpp
 * Uses factory methods for one-liner pin configuration.
 *
 * Hardware:
 * - LED on PA5 (Nucleo F401/G071/G0B1) or PB7 (Nucleo F722)
 *
 * Expected behavior:
 * - LED blinks at 1 Hz (500ms on, 500ms off)
 *
 * @note Part of MicroCore API Tier Examples
 */

#include "board.hpp"
#include "hal/api/gpio_simple.hpp"

using namespace ucore::hal;
using namespace board::pins;

// Simple delay helper
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 10000; i++) {
        __asm volatile("nop");
    }
}

int main() {
    // Simple API: One-liner factory method
    // Creates an active-high output pin, automatically configured
    auto led = Gpio::output<led_green>();

    // Initial state is OFF
    while (true) {
        led.on();       // Turn LED on
        delay_ms(500);  // Wait 500ms

        led.off();      // Turn LED off
        delay_ms(500);  // Wait 500ms
    }

    return 0;
}
