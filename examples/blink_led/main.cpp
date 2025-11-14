/**
 * @file main.cpp
 * @brief Blink LED Example for SAME70 Xplained Ultra
 *
 * This example demonstrates the use of CoreZero HAL abstractions:
 * - Board initialization (clock, SysTick, peripherals)
 * - GPIO configuration using platform-specific GpioPin template
 * - SysTick-based millisecond delays
 *
 * @section hardware Hardware Setup
 * - Board: SAME70 Xplained Ultra
 * - LED: Green LED on PC8 (PIOC pin 8) - Active LOW
 * - Clock: 12 MHz internal RC oscillator (PLL not available - see docs/KNOWN_ISSUES.md)
 *
 * @section behavior Expected Behavior
 * The LED blinks with 500ms ON / 500ms OFF period (1 Hz frequency).
 *
 * @note This example uses the SAME70 platform-specific GPIO API.
 *       For portable code across multiple platforms, use the hal::interface APIs.
 */

#include "same70_xplained/board.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

using namespace alloy::hal::same70;
using namespace alloy::generated::atsame70q21b;

int main() {
    // Initialize board hardware (clock, SysTick, interrupts, peripherals)
    board::init();

    // Create LED GPIO pin instance (PC8)
    // Using platform-specific GpioPin template for compile-time peripheral configuration
    GpioPin<peripherals::PIOC, 8> led;
    led.setDirection(PinDirection::Output);

    // Main loop: Blink LED at 1 Hz
    while (true) {
        led.clear();  // LED ON (active LOW)
        board::delay_ms(500);
        led.set();    // LED OFF
        board::delay_ms(500);
    }

    return 0;
}
