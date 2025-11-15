/**
 * @file hw_gpio_led_test.cpp
 * @brief Hardware validation test for GPIO LED control
 *
 * This test validates GPIO and Clock functionality on real hardware by:
 * 1. Initializing system clock
 * 2. Configuring LED GPIO pin
 * 3. Blinking LED in a pattern
 *
 * SUCCESS: LED blinks in expected pattern
 * FAILURE: LED doesn't blink or blinks incorrectly
 *
 * @note This test requires actual hardware to run
 * @note Visual verification required - watch the LED!
 */

#include "core/result.hpp"
#include "core/error.hpp"

using namespace alloy::core;

// ==============================================================================
// Platform-Specific Includes
// ==============================================================================

#if defined(ALLOY_BOARD_NUCLEO_G0B1RE) || defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "hal/vendors/st/stm32g0/clock_platform.hpp"
    #include "hal/vendors/st/stm32g0/gpio.hpp"
    #include "boards/board_config.hpp"

    using ClockPlatform = alloy::hal::st::stm32g0::Stm32g0Clock<
        alloy::hal::st::stm32g0::ExampleG0ClockConfig
    >;
    // Use board-specific LED pin from board_config.hpp
    using LedPin = alloy::boards::LedGreen;

#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "hal/vendors/st/stm32f4/clock_platform.hpp"
    #include "hal/vendors/st/stm32f4/gpio.hpp"
    #include "boards/board_config.hpp"

    using ClockPlatform = alloy::hal::st::stm32f4::Stm32f4Clock<
        alloy::hal::st::stm32f4::ExampleF4ClockConfig
    >;
    using LedPin = alloy::boards::LedGreen;

#elif defined(ALLOY_BOARD_NUCLEO_F722ZE)
    #include "hal/vendors/st/stm32f7/clock_platform.hpp"
    #include "hal/vendors/st/stm32f7/gpio.hpp"
    #include "boards/board_config.hpp"

    using ClockPlatform = alloy::hal::st::stm32f7::Stm32f7Clock<
        alloy::hal::st::stm32f7::ExampleF7ClockConfig
    >;
    using LedPin = alloy::boards::LedGreen;

#else
    #error "Unsupported board for hardware LED test"
#endif

// ==============================================================================
// Simple Delay Function
// ==============================================================================

/**
 * @brief Simple busy-wait delay
 * @param cycles Number of cycles to delay
 * @note This is not accurate timing - for visual LED blinking only
 */
inline void delay_cycles(volatile uint32_t cycles) {
    while(cycles--) {
        __asm__("nop");
    }
}

/**
 * @brief Approximate millisecond delay
 * @param ms Milliseconds to delay
 */
void delay_ms(uint32_t ms) {
    // Rough approximation: adjust based on actual clock speed
    uint32_t freq = ClockPlatform::get_system_clock_hz();
    uint32_t cycles_per_ms = freq / 1000 / 4; // Divide by 4 for instruction overhead

    for(uint32_t i = 0; i < ms; i++) {
        delay_cycles(cycles_per_ms);
    }
}

// ==============================================================================
// Test Assertions
// ==============================================================================

/**
 * @brief Simple assertion that hangs on failure
 * @param condition Condition to check
 * @note On embedded, we can't print - just hang with LED pattern
 */
#define HW_ASSERT(condition) \
    if (!(condition)) { \
        /* Failure pattern: rapid blink */ \
        while(1) { \
            led.toggle(); \
            delay_ms(100); \
        } \
    }

// ==============================================================================
// Hardware Test Main
// ==============================================================================

LedPin led; // Global LED instance

/**
 * @brief Main hardware test entry point
 * @return 0 on success (test passed)
 *
 * Test sequence:
 * 1. Initialize system clock
 * 2. Enable GPIO clocks
 * 3. Configure LED pin as output
 * 4. Blink LED in pattern to indicate success
 *
 * Pattern: 3 fast blinks, pause, repeat
 */
int main() {
    // Step 1: Initialize system clock
    auto clock_result = ClockPlatform::initialize();
    HW_ASSERT(clock_result.is_ok());

    // Step 2: Enable GPIO clocks
    auto gpio_clocks_result = ClockPlatform::enable_gpio_clocks();
    HW_ASSERT(gpio_clocks_result.is_ok());

    // Step 3: Configure LED pin as output
    auto direction_result = led.setDirection(alloy::hal::PinDirection::Output);
    HW_ASSERT(direction_result.is_ok());

    auto drive_result = led.setDrive(alloy::hal::PinDrive::PushPull);
    HW_ASSERT(drive_result.is_ok());

    // Step 4: Initial state - LED OFF
    auto clear_result = led.clear();
    HW_ASSERT(clear_result.is_ok());

    delay_ms(500); // Initial pause

    // ==============================================================================
    // Main Test Loop: Blink Pattern
    // ==============================================================================

    while(1) {
        // Pattern: 3 fast blinks (test passed!)
        for(int i = 0; i < 3; i++) {
            auto set_result = led.set();
            HW_ASSERT(set_result.is_ok());
            delay_ms(100);

            auto clear_result2 = led.clear();
            HW_ASSERT(clear_result2.is_ok());
            delay_ms(100);
        }

        // Long pause between patterns
        delay_ms(1000);
    }

    return 0; // Never reached
}

// ==============================================================================
// Test Documentation
// ==============================================================================

/**
 * @test GPIO LED Blink Test
 *
 * @testcase TC-HW-GPIO-001
 * @objective Validate GPIO and Clock initialization on hardware
 *
 * @precondition
 * - Board powered on
 * - LED connected to configured GPIO pin
 * - No other code running
 *
 * @procedure
 * 1. Flash this test to board
 * 2. Reset board
 * 3. Observe LED behavior
 *
 * @expected
 * LED should blink in pattern: 3 quick blinks, 1 second pause, repeat
 *
 * @actual
 * [To be filled during manual testing]
 *
 * @result
 * - PASS: LED blinks in expected pattern
 * - FAIL: LED doesn't blink or shows rapid blink (assertion failure)
 *
 * @notes
 * - This test requires visual verification
 * - Rapid continuous blinking indicates assertion failure
 * - Check which assertion failed by debugging
 */
