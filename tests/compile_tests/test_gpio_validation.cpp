/**
 * @file test_gpio_validation.cpp
 * @brief Compile-time validation tests for GPIO pins
 *
 * This file contains tests to verify that invalid GPIO pins are rejected
 * at compile time. These are NOT runtime tests - they test compile-time
 * validation using static_assert.
 *
 * To test validation failure:
 * 1. Uncomment one of the "SHOULD_FAIL" test cases
 * 2. Try to compile - it should fail with a clear error message
 * 3. Re-comment the test case
 */

#ifdef ALLOY_MCU  // Only compile when building for real hardware

    #include "hal/st/stm32f1/gpio.hpp"

using namespace alloy::hal::stm32f1;

// ============================================================================
// Valid GPIO Pin Tests (these should compile successfully)
// ============================================================================

// Test valid pins on STM32F103C8 (48-pin package)
// STM32F103C8 has GPIOA, GPIOB, GPIOC (7 ports total in database)
void test_valid_gpio_pins() {
    // Valid pins on Port A (PA0-PA15)
    GpioPin<0> pa0;    // PA0 - valid
    GpioPin<15> pa15;  // PA15 - valid

    // Valid pins on Port B (PB0-PB15)
    GpioPin<16> pb0;   // PB0 - valid
    GpioPin<31> pb15;  // PB15 - valid

    // Valid pins on Port C (PC0-PC15)
    GpioPin<32> pc0;   // PC0 - valid
    GpioPin<45> pc13;  // PC13 - valid (BluePill LED)
}

// ============================================================================
// Invalid GPIO Pin Tests (uncomment to test compile-time validation)
// ============================================================================

// SHOULD_FAIL: Pin 112 doesn't exist (max is 111)
// Uncomment to test:
// void test_invalid_pin_out_of_range() {
//     GpioPin<112> invalid_pin;  // Should fail with "GPIO pin not available"
// }

// SHOULD_FAIL: Port G doesn't exist on STM32F103C8
// Uncomment to test:
// void test_invalid_port() {
//     GpioPin<112> pg0;  // Port G = 112/16 = 7, should fail with "GPIO port not available"
// }

#endif  // ALLOY_MCU
