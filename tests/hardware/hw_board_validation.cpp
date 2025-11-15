/**
 * @file hw_board_validation.cpp
 * @brief Hardware validation test for board-specific functionality
 *
 * This test validates board-specific features:
 * 1. All LEDs functional
 * 2. Button input working
 * 3. Board identification
 * 4. Pin configuration
 *
 * SUCCESS: Sequential LED pattern, button interaction works
 * FAILURE: LEDs don't light or button doesn't respond
 *
 * @note This test is board-specific and tests actual board features
 */

#include "core/result.hpp"
#include "core/error.hpp"

using namespace alloy::core;
using namespace alloy::hal;

// ==============================================================================
// Platform-Specific Includes
// ==============================================================================

#if defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #define BOARD_NAME "Nucleo-G0B1RE"
    #define PLATFORM_NAME "STM32G0"

    #include "hal/vendors/st/stm32g0/clock_platform.hpp"
    #include "hal/vendors/st/stm32g0/gpio.hpp"

    using ClockPlatform = alloy::hal::st::stm32g0::Stm32g0Clock<
        alloy::hal::st::stm32g0::ExampleG0ClockConfig
    >;

    // Nucleo-G0B1RE has 1 LED (PA5 - Green) and 1 Button (PC13)
    using LedGreen = alloy::hal::st::stm32g0::GpioPin<
        alloy::hal::st::stm32g0::gpio::PortA, 5
    >;
    using ButtonUser = alloy::hal::st::stm32g0::GpioPin<
        alloy::hal::st::stm32g0::gpio::PortC, 13
    >;

    constexpr int LED_COUNT = 1;

#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #define BOARD_NAME "Nucleo-G071RB"
    #define PLATFORM_NAME "STM32G0"

    #include "hal/vendors/st/stm32g0/clock_platform.hpp"
    #include "hal/vendors/st/stm32g0/gpio.hpp"

    using ClockPlatform = alloy::hal::st::stm32g0::Stm32g0Clock<
        alloy::hal::st::stm32g0::ExampleG0ClockConfig
    >;

    using LedGreen = alloy::hal::st::stm32g0::GpioPin<
        alloy::hal::st::stm32g0::gpio::PortA, 5
    >;
    using ButtonUser = alloy::hal::st::stm32g0::GpioPin<
        alloy::hal::st::stm32g0::gpio::PortC, 13
    >;

    constexpr int LED_COUNT = 1;

#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #define BOARD_NAME "Nucleo-F401RE"
    #define PLATFORM_NAME "STM32F4"

    #include "hal/vendors/st/stm32f4/clock_platform.hpp"
    #include "hal/vendors/st/stm32f4/gpio.hpp"

    using ClockPlatform = alloy::hal::st::stm32f4::Stm32f4Clock<
        alloy::hal::st::stm32f4::ExampleF4ClockConfig
    >;

    using LedGreen = alloy::hal::st::stm32f4::GpioPin<
        alloy::hal::st::stm32f4::gpio::PortA, 5
    >;
    using ButtonUser = alloy::hal::st::stm32f4::GpioPin<
        alloy::hal::st::stm32f4::gpio::PortC, 13
    >;

    constexpr int LED_COUNT = 1;

#elif defined(ALLOY_BOARD_NUCLEO_F722ZE)
    #define BOARD_NAME "Nucleo-F722ZE"
    #define PLATFORM_NAME "STM32F7"

    #include "hal/vendors/st/stm32f7/clock_platform.hpp"
    #include "hal/vendors/st/stm32f7/gpio.hpp"

    using ClockPlatform = alloy::hal::st::stm32f7::Stm32f7Clock<
        alloy::hal::st::stm32f7::ExampleF7ClockConfig
    >;

    // Nucleo-F722ZE has 3 LEDs
    using LedGreen  = alloy::hal::st::stm32f7::GpioPin<
        alloy::hal::st::stm32f7::gpio::PortB, 0
    >;
    using LedBlue   = alloy::hal::st::stm32f7::GpioPin<
        alloy::hal::st::stm32f7::gpio::PortB, 7
    >;
    using LedRed    = alloy::hal::st::stm32f7::GpioPin<
        alloy::hal::st::stm32f7::gpio::PortB, 14
    >;
    using ButtonUser = alloy::hal::st::stm32f7::GpioPin<
        alloy::hal::st::stm32f7::gpio::PortC, 13
    >;

    constexpr int LED_COUNT = 3;

#else
    #error "Unsupported board for hardware board validation test"
#endif

// ==============================================================================
// Timing Functions
// ==============================================================================

inline void delay_cycles(volatile uint32_t cycles) {
    while(cycles--) {
        __asm__("nop");
    }
}

void delay_ms(uint32_t ms) {
    uint32_t freq = ClockPlatform::get_system_clock_hz();
    uint32_t cycles_per_ms = freq / 1000 / 4;

    for(uint32_t i = 0; i < ms; i++) {
        delay_cycles(cycles_per_ms);
    }
}

// ==============================================================================
// Test Helpers
// ==============================================================================

LedGreen led_green;
#if LED_COUNT >= 3
LedBlue led_blue;
LedRed led_red;
#endif
ButtonUser button;

#define HW_ASSERT(condition) \
    if (!(condition)) { \
        while(1) { \
            led_green.toggle(); \
            delay_ms(50); \
        } \
    }

// ==============================================================================
// Board Tests
// ==============================================================================

/**
 * @brief Test all LEDs on the board
 */
void test_all_leds() {
    // Configure all LEDs as outputs
    led_green.setDirection(PinDirection::Output);
    led_green.setDrive(PinDrive::PushPull);

#if LED_COUNT >= 3
    led_blue.setDirection(PinDirection::Output);
    led_blue.setDrive(PinDrive::PushPull);

    led_red.setDirection(PinDirection::Output);
    led_red.setDrive(PinDrive::PushPull);
#endif

    // Test each LED individually
    led_green.set();
    delay_ms(500);
    led_green.clear();
    delay_ms(200);

#if LED_COUNT >= 3
    led_blue.set();
    delay_ms(500);
    led_blue.clear();
    delay_ms(200);

    led_red.set();
    delay_ms(500);
    led_red.clear();
    delay_ms(200);
#endif
}

/**
 * @brief Test button input
 * @return true if button can be read
 */
bool test_button() {
    // Configure button as input with pull-up
    auto dir_result = button.setDirection(PinDirection::Input);
    if (dir_result.is_err()) {
        return false;
    }

    auto pull_result = button.setPull(PinPull::PullUp);
    if (pull_result.is_err()) {
        return false;
    }

    // Button should read HIGH when not pressed (pull-up)
    auto read_result = button.read();
    if (read_result.is_err()) {
        return false;
    }

    return true;
}

/**
 * @brief Run LED sequence pattern
 */
void led_sequence_pattern() {
#if LED_COUNT >= 3
    // 3-LED chase pattern
    for(int i = 0; i < 3; i++) {
        led_green.set();
        delay_ms(150);
        led_green.clear();

        led_blue.set();
        delay_ms(150);
        led_blue.clear();

        led_red.set();
        delay_ms(150);
        led_red.clear();
    }
#else
    // Single LED blink pattern
    for(int i = 0; i < 5; i++) {
        led_green.set();
        delay_ms(200);
        led_green.clear();
        delay_ms(200);
    }
#endif
}

// ==============================================================================
// Main Test
// ==============================================================================

/**
 * @brief Main hardware test entry point
 *
 * Test sequence:
 * 1. Initialize system
 * 2. Test all LEDs
 * 3. Test button input
 * 4. Interactive mode: button press lights LED
 */
int main() {
    // Step 1: Initialize system clock
    auto clock_result = ClockPlatform::initialize();
    HW_ASSERT(clock_result.is_ok());

    auto gpio_result = ClockPlatform::enable_gpio_clocks();
    HW_ASSERT(gpio_result.is_ok());

    // Step 2: Test all LEDs
    test_all_leds();

    delay_ms(1000);

    // Step 3: LED sequence to indicate board validation start
    led_sequence_pattern();

    // Step 4: Test button
    HW_ASSERT(test_button());

    // ==============================================================================
    // Interactive Mode: Button → LED
    // ==============================================================================

    /**
     * Interactive test:
     * - Press button → Green LED lights
     * - Release button → Green LED off
     *
     * This validates button input and LED output simultaneously
     */

    while(1) {
        auto button_state = button.read();

        if (button_state.is_ok()) {
            // Button is active LOW (pulled up, pressed = LOW)
            if (button_state.unwrap() == 0) {
                // Button pressed - light LED
                led_green.set();

#if LED_COUNT >= 3
                // Also light blue LED on F722ZE
                led_blue.set();
#endif
            } else {
                // Button not pressed - LED off
                led_green.clear();

#if LED_COUNT >= 3
                led_blue.clear();
#endif
            }
        }

        delay_ms(10); // Debounce delay
    }

    return 0;
}

// ==============================================================================
// Test Documentation
// ==============================================================================

/**
 * @test Board-Specific Validation Test
 *
 * @testcase TC-HW-BOARD-001
 * @objective Validate board-specific hardware features
 *
 * @boards
 * - Nucleo-G0B1RE: 1 LED, 1 Button
 * - Nucleo-G071RB: 1 LED, 1 Button
 * - Nucleo-F401RE: 1 LED, 1 Button
 * - Nucleo-F722ZE: 3 LEDs, 1 Button
 *
 * @procedure
 * 1. Flash test to board
 * 2. Reset board
 * 3. Observe LED sequence (all LEDs tested individually)
 * 4. Wait for chase/blink pattern
 * 5. Press and hold blue user button
 * 6. Verify LED lights while button pressed
 *
 * @expected
 * - Each LED lights for 500ms sequentially
 * - Chase/blink pattern executes
 * - Button press lights LED
 * - Button release turns off LED
 *
 * @result
 * - PASS: All expected behaviors observed
 * - FAIL: LED doesn't light, button doesn't work, or rapid blink (assertion)
 *
 * @notes
 * - Rapid blinking indicates assertion failure
 * - Test is interactive - requires button press
 * - Different boards have different LED counts
 */
