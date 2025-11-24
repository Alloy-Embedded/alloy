/**
 * @file fluent_gpio_blink.cpp
 * @brief GPIO Fluent API Example - LED Blink with Builder Pattern
 *
 * Demonstrates the Fluent tier API from hal/api/gpio_fluent.hpp
 * Uses method chaining for explicit, self-documenting configuration.
 *
 * Hardware:
 * - LED on PA5 (Nucleo F401/G071/G0B1) or PB7 (Nucleo F722)
 *
 * Expected behavior:
 * - LED blinks with explicit configuration
 * - Uses Result<T> for error handling
 *
 * @note Part of MicroCore API Tier Examples
 */

#include "board.hpp"
#include "hal/api/gpio_fluent.hpp"

using namespace ucore::hal;
using namespace board::pins;

// Delay helper
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 10000; i++) {
        __asm volatile("nop");
    }
}

int main() {
    // Fluent API: Builder pattern with method chaining
    // Each method returns *this for chaining
    auto led = GpioFluent<led_green>()
        .as_output()           // Set direction to output
        .push_pull()           // Drive mode: push-pull
        .active_high()         // Logical level: active-high
        .initial_state_off()   // Start with LED off
        .build();              // Build the configured pin

    while (true) {
        // Result-based error handling with .unwrap()
        led.on().unwrap();
        delay_ms(500);

        led.off().unwrap();
        delay_ms(500);
    }

    return 0;
}
