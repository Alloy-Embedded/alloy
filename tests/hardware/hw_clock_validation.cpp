/**
 * @file hw_clock_validation.cpp
 * @brief Hardware validation test for system clock configuration
 *
 * This test validates clock initialization and frequency on real hardware by:
 * 1. Initializing system clock to target frequency
 * 2. Verifying clock frequency is set correctly
 * 3. Testing peripheral clock enables
 * 4. Using LED blink rate to visually verify timing
 *
 * SUCCESS: LED blinks at exactly 1 Hz (1 second ON, 1 second OFF)
 * FAILURE: LED blinks too fast/slow or doesn't blink
 *
 * @note This test requires actual hardware to run
 * @note Timing can be verified with stopwatch
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
    using LedPin = alloy::boards::LedGreen;

    constexpr uint32_t EXPECTED_FREQ_HZ = 64'000'000; // 64 MHz

#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "hal/vendors/st/stm32f4/clock_platform.hpp"
    #include "hal/vendors/st/stm32f4/gpio.hpp"
    #include "boards/board_config.hpp"

    using ClockPlatform = alloy::hal::st::stm32f4::Stm32f4Clock<
        alloy::hal::st::stm32f4::ExampleF4ClockConfig
    >;
    using LedPin = alloy::boards::LedGreen;

    constexpr uint32_t EXPECTED_FREQ_HZ = 84'000'000; // 84 MHz

#elif defined(ALLOY_BOARD_NUCLEO_F722ZE)
    #include "hal/vendors/st/stm32f7/clock_platform.hpp"
    #include "hal/vendors/st/stm32f7/gpio.hpp"
    #include "boards/board_config.hpp"

    using ClockPlatform = alloy::hal::st::stm32f7::Stm32f7Clock<
        alloy::hal::st::stm32f7::ExampleF7ClockConfig
    >;
    using LedPin = alloy::boards::LedGreen;

    constexpr uint32_t EXPECTED_FREQ_HZ = 216'000'000; // 216 MHz

#else
    #error "Unsupported board for hardware clock test"
#endif

// ==============================================================================
// Timing Functions
// ==============================================================================

/**
 * @brief Calibrated delay using system clock frequency
 * @param cycles Number of CPU cycles to delay
 */
inline void delay_cycles(volatile uint32_t cycles) {
    while(cycles--) {
        __asm__("nop");
    }
}

/**
 * @brief Accurate millisecond delay based on actual clock frequency
 * @param ms Milliseconds to delay
 */
void delay_ms_accurate(uint32_t ms) {
    uint32_t freq = ClockPlatform::get_system_clock_hz();

    // Calculate cycles per millisecond
    // Divide by 4 to account for loop overhead
    uint32_t cycles_per_ms = freq / 1000 / 4;

    for(uint32_t i = 0; i < ms; i++) {
        delay_cycles(cycles_per_ms);
    }
}

// ==============================================================================
// Test Helpers
// ==============================================================================

LedPin led;

#define HW_ASSERT(condition) \
    if (!(condition)) { \
        /* Failure: rapid blink */ \
        while(1) { \
            led.toggle(); \
            delay_ms_accurate(50); \
        } \
    }

// ==============================================================================
// Clock Validation Tests
// ==============================================================================

/**
 * @brief Validate system clock frequency
 * @return true if frequency matches expected value
 */
bool validate_clock_frequency() {
    uint32_t actual_freq = ClockPlatform::get_system_clock_hz();
    return (actual_freq == EXPECTED_FREQ_HZ);
}

/**
 * @brief Test peripheral clock enables
 * @return true if all peripheral clocks can be enabled
 */
bool test_peripheral_clocks() {
    // Test GPIO clocks
    auto gpio_result = ClockPlatform::enable_gpio_clocks();
    if (gpio_result.is_err()) {
        return false;
    }

    // Test UART clock (if supported)
    auto uart_result = ClockPlatform::enable_uart_clock(0x40013800); // USART1 base
    if (uart_result.is_err()) {
        return false;
    }

    // Test SPI clock (if supported)
    auto spi_result = ClockPlatform::enable_spi_clock(0x40013000); // SPI1 base
    if (spi_result.is_err()) {
        return false;
    }

    // Test I2C clock (if supported)
    auto i2c_result = ClockPlatform::enable_i2c_clock(0x40005400); // I2C1 base
    if (i2c_result.is_err()) {
        return false;
    }

    return true;
}

// ==============================================================================
// Main Test
// ==============================================================================

/**
 * @brief Main hardware test entry point
 *
 * Test sequence:
 * 1. Initialize system clock
 * 2. Validate clock frequency
 * 3. Enable and test peripheral clocks
 * 4. Visual timing validation via LED (1 Hz blink)
 */
int main() {
    // Step 1: Initialize system clock
    auto clock_result = ClockPlatform::initialize();
    HW_ASSERT(clock_result.is_ok());

    // Step 2: Validate clock frequency
    HW_ASSERT(validate_clock_frequency());

    // Step 3: Test peripheral clocks
    HW_ASSERT(test_peripheral_clocks());

    // Step 4: Configure LED for visual validation
    auto direction_result = led.setDirection(alloy::hal::PinDirection::Output);
    HW_ASSERT(direction_result.is_ok());

    auto drive_result = led.setDrive(alloy::hal::PinDrive::PushPull);
    HW_ASSERT(drive_result.is_ok());

    led.clear();

    // ==============================================================================
    // Visual Timing Validation
    // ==============================================================================

    /**
     * Blink at exactly 1 Hz to allow timing verification with stopwatch
     *
     * Expected behavior:
     * - LED ON for 1000ms
     * - LED OFF for 1000ms
     * - Total period: 2000ms (0.5 Hz square wave)
     *
     * Use stopwatch to count: should match real time!
     */

    while(1) {
        // LED ON for 1 second
        led.set();
        delay_ms_accurate(1000);

        // LED OFF for 1 second
        led.clear();
        delay_ms_accurate(1000);
    }

    return 0;
}

// ==============================================================================
// Test Documentation
// ==============================================================================

/**
 * @test Clock Frequency Validation Test
 *
 * @testcase TC-HW-CLOCK-001
 * @objective Validate system clock configuration and timing accuracy
 *
 * @precondition
 * - Board powered on
 * - External crystal/oscillator functional (if used)
 * - LED connected to configured GPIO pin
 *
 * @procedure
 * 1. Flash this test to board
 * 2. Reset board
 * 3. Use stopwatch to time LED blinks
 * 4. Count number of blinks in 60 seconds
 *
 * @expected
 * - LED should blink exactly 30 times in 60 seconds (1 Hz)
 * - Each blink: 1 second ON, 1 second OFF
 * - Timing should be accurate to within 1%
 *
 * @actual
 * [To be filled during manual testing]
 * Blinks counted in 60s: ____
 * Timing accuracy: ____
 *
 * @result
 * - PASS: 29-31 blinks in 60 seconds (±3% tolerance)
 * - FAIL: Outside tolerance or rapid blink (assertion failure)
 *
 * @notes
 * - Rapid blinking indicates test assertion failure
 * - Slow/fast blinking indicates clock configuration error
 * - Use digital stopwatch for best accuracy
 *
 * @validation_criteria
 * 1. Clock frequency API returns expected value
 * 2. All peripheral clocks can be enabled
 * 3. Timing matches real-world clock (stopwatch)
 * 4. No drift over time (run for 5+ minutes)
 */
