/**
 * @file test_hal_concepts.cpp
 * @brief Unit tests for HAL peripheral C++20 concepts
 *
 * Tests the concept definitions for peripheral validation.
 * Part of Phase 1: Core Concept Definitions
 *
 * @see openspec/changes/modernize-peripheral-architecture/specs/concept-layer/spec.md
 */

#include <cassert>
#include <iostream>
#include <span>
#include <string>

#include "../../src/core/error.hpp"
#include "../../src/core/result.hpp"
#include "../../src/core/types.hpp"
#include "../../src/hal/concepts.hpp"
#include "../../src/hal/platform/same70/gpio.hpp"
#include "../../src/hal/types.hpp"

using namespace alloy::core;
using namespace alloy::hal;
using namespace alloy::hal::concepts;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                \
    void test_##name();                                           \
    void run_test_##name() {                                      \
        tests_run++;                                              \
        std::cout << "Running test: " #name << "...";             \
        try {                                                     \
            test_##name();                                        \
            tests_passed++;                                       \
            std::cout << " PASS" << std::endl;                    \
        } catch (const std::exception& e) {                       \
            std::cout << " FAIL: " << e.what() << std::endl;      \
        } catch (...) {                                           \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        }                                                         \
    }                                                             \
    void test_##name()

#define ASSERT(condition)                                              \
    do {                                                               \
        if (!(condition)) {                                            \
            throw std::runtime_error("Assertion failed: " #condition); \
        }                                                              \
    } while (0)

// =============================================================================
// Mock Types for Testing
// =============================================================================

/**
 * @brief Mock GPIO pin that satisfies GpioPin concept
 */
struct MockValidGpioPin {
    static constexpr u32 port_base = 0x400E0E00;
    static constexpr u8 pin_number = 0;
    static constexpr u32 pin_mask = 1;

    Result<void, ErrorCode> set() { return Ok(); }
    Result<void, ErrorCode> clear() { return Ok(); }
    Result<void, ErrorCode> toggle() { return Ok(); }
    Result<void, ErrorCode> write(bool value) { return Ok(); }
    Result<bool, ErrorCode> read() const { return Ok(true); }
    Result<bool, ErrorCode> isOutput() const { return Ok(true); }
    Result<void, ErrorCode> setDirection(PinDirection dir) { return Ok(); }
    Result<void, ErrorCode> setDrive(PinDrive drive) { return Ok(); }
    Result<void, ErrorCode> setPull(PinPull pull) { return Ok(); }
};

/**
 * @brief Mock GPIO pin with missing methods (should NOT satisfy concept)
 */
struct MockInvalidGpioPin {
    static constexpr u32 port_base = 0x400E0E00;
    static constexpr u8 pin_number = 0;
    static constexpr u32 pin_mask = 1;

    // Missing: set(), clear(), toggle(), etc.
    void some_other_method() {}
};

/**
 * @brief Mock UART that satisfies UartPeripheral concept
 */
struct MockValidUart {
    Result<u8, ErrorCode> read_byte() { return Ok(u8(0)); }
    Result<void, ErrorCode> write_byte(u8 byte) { return Ok(); }
    usize available() const { return 0; }
};

/**
 * @brief Mock UART with wrong signatures (should NOT satisfy concept)
 */
struct MockInvalidUart {
    void read_byte() {}  // Wrong return type
    void write_byte(u8 byte) {}  // Wrong return type
    int available() { return 0; }  // Not const
};

/**
 * @brief Mock SPI that satisfies SpiPeripheral concept
 */
struct MockValidSpi {
    Result<void, ErrorCode> transfer(std::span<const u8> tx, std::span<u8> rx) {
        return Ok();
    }
    Result<void, ErrorCode> transmit(std::span<const u8> tx) { return Ok(); }
    Result<void, ErrorCode> receive(std::span<u8> rx) { return Ok(); }
    bool is_busy() const { return false; }
};

/**
 * @brief Mock Timer that satisfies TimerPeripheral concept
 */
struct MockValidTimer {
    Result<void, ErrorCode> start() { return Ok(); }
    Result<void, ErrorCode> stop() { return Ok(); }
    Result<void, ErrorCode> set_period(u32 period) { return Ok(); }
    Result<u32, ErrorCode> get_count() const { return Ok(u32(0)); }
    bool is_running() const { return false; }
};

/**
 * @brief Mock ADC that satisfies AdcPeripheral concept
 */
struct MockValidAdc {
    Result<u16, ErrorCode> read() { return Ok(u16(0)); }
    Result<void, ErrorCode> start_conversion() { return Ok(); }
    bool is_conversion_complete() const { return false; }
};

/**
 * @brief Mock PWM that satisfies PwmPeripheral concept
 */
struct MockValidPwm {
    Result<void, ErrorCode> set_frequency(u32 freq) { return Ok(); }
    Result<void, ErrorCode> set_duty_cycle(u32 duty) { return Ok(); }
    Result<void, ErrorCode> start() { return Ok(); }
    Result<void, ErrorCode> stop() { return Ok(); }
};

/**
 * @brief Mock peripheral with interrupt capability
 */
struct MockInterruptCapable {
    Result<void, ErrorCode> enable_interrupt() { return Ok(); }
    Result<void, ErrorCode> disable_interrupt() { return Ok(); }
    Result<void, ErrorCode> clear_interrupt() { return Ok(); }
};

/**
 * @brief Mock peripheral with DMA capability
 */
struct MockDmaCapable {
    Result<void, ErrorCode> enable_dma() { return Ok(); }
    Result<void, ErrorCode> disable_dma() { return Ok(); }
    usize get_data_register_address() { return 0x40000000; }
};

// =============================================================================
// GpioPin Concept Tests
// =============================================================================

TEST(gpio_pin_valid_mock) {
    // Mock type should satisfy GpioPin concept
    ASSERT(GpioPin<MockValidGpioPin>);
}

TEST(gpio_pin_invalid_mock) {
    // Mock with missing methods should NOT satisfy concept
    ASSERT(!GpioPin<MockInvalidGpioPin>);
}

TEST(gpio_pin_not_integer) {
    // Basic types should NOT satisfy GpioPin concept
    ASSERT(!GpioPin<int>);
    ASSERT(!GpioPin<u32>);
}

TEST(gpio_pin_real_implementation) {
    // Real GPIO pin from platform layer should satisfy concept
    using RealPin = alloy::hal::same70::GpioPin<0x400E0E00, 0>;
    ASSERT(GpioPin<RealPin>);
}

// =============================================================================
// UartPeripheral Concept Tests
// =============================================================================

TEST(uart_peripheral_valid_mock) {
    ASSERT(UartPeripheral<MockValidUart>);
}

TEST(uart_peripheral_invalid_mock) {
    ASSERT(!UartPeripheral<MockInvalidUart>);
}

TEST(uart_peripheral_not_basic_type) {
    ASSERT(!UartPeripheral<int>);
    ASSERT(!UartPeripheral<void*>);
}

// =============================================================================
// SpiPeripheral Concept Tests
// =============================================================================

TEST(spi_peripheral_valid_mock) {
    ASSERT(SpiPeripheral<MockValidSpi>);
}

TEST(spi_peripheral_not_basic_type) {
    ASSERT(!SpiPeripheral<int>);
}

// =============================================================================
// TimerPeripheral Concept Tests
// =============================================================================

TEST(timer_peripheral_valid_mock) {
    ASSERT(TimerPeripheral<MockValidTimer>);
}

TEST(timer_peripheral_not_basic_type) {
    ASSERT(!TimerPeripheral<int>);
}

// =============================================================================
// AdcPeripheral Concept Tests
// =============================================================================

TEST(adc_peripheral_valid_mock) {
    ASSERT(AdcPeripheral<MockValidAdc>);
}

TEST(adc_peripheral_not_basic_type) {
    ASSERT(!AdcPeripheral<int>);
}

// =============================================================================
// PwmPeripheral Concept Tests
// =============================================================================

TEST(pwm_peripheral_valid_mock) {
    ASSERT(PwmPeripheral<MockValidPwm>);
}

TEST(pwm_peripheral_not_basic_type) {
    ASSERT(!PwmPeripheral<int>);
}

// =============================================================================
// Capability Concept Tests
// =============================================================================

TEST(interrupt_capable_valid_mock) {
    ASSERT(InterruptCapable<MockInterruptCapable>);
}

TEST(interrupt_capable_not_basic_type) {
    ASSERT(!InterruptCapable<int>);
}

TEST(dma_capable_valid_mock) {
    ASSERT(DmaCapable<MockDmaCapable>);
}

TEST(dma_capable_not_basic_type) {
    ASSERT(!DmaCapable<int>);
}

// =============================================================================
// Concept-Constrained Function Tests
// =============================================================================

/**
 * @brief Example function using GpioPin concept
 */
template <GpioPin Pin>
void blink_led(Pin& pin) {
    pin.toggle();
}

TEST(concept_constrained_gpio_function) {
    MockValidGpioPin pin;
    blink_led(pin);  // Should compile fine
    ASSERT(true);
}

/**
 * @brief Example function using UartPeripheral concept
 */
template <UartPeripheral Uart>
void send_byte(Uart& uart, u8 byte) {
    uart.write_byte(byte);
}

TEST(concept_constrained_uart_function) {
    MockValidUart uart;
    send_byte(uart, 0x42);  // Should compile fine
    ASSERT(true);
}

/**
 * @brief Example function using multiple concepts
 */
template <typename T>
    requires GpioPin<T> && InterruptCapable<T>
void configure_pin_interrupt(T& pin) {
    pin.setDirection(PinDirection::Input);
    pin.enable_interrupt();
}

// =============================================================================
// Compile-time Static Assertions
// =============================================================================

// These verify concepts at compile time
static_assert(GpioPin<MockValidGpioPin>, "MockValidGpioPin must satisfy GpioPin");
static_assert(!GpioPin<int>, "int must NOT satisfy GpioPin");
static_assert(UartPeripheral<MockValidUart>, "MockValidUart must satisfy UartPeripheral");
static_assert(!UartPeripheral<int>, "int must NOT satisfy UartPeripheral");
static_assert(SpiPeripheral<MockValidSpi>, "MockValidSpi must satisfy SpiPeripheral");
static_assert(TimerPeripheral<MockValidTimer>, "MockValidTimer must satisfy TimerPeripheral");
static_assert(AdcPeripheral<MockValidAdc>, "MockValidAdc must satisfy AdcPeripheral");
static_assert(PwmPeripheral<MockValidPwm>, "MockValidPwm must satisfy PwmPeripheral");

TEST(compile_time_concept_checks) {
    // If we reach here, all static_assert passed
    ASSERT(true);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  HAL Concepts Unit Tests (C++20)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run GPIO concept tests
    run_test_gpio_pin_valid_mock();
    run_test_gpio_pin_invalid_mock();
    run_test_gpio_pin_not_integer();
    run_test_gpio_pin_real_implementation();

    // Run UART concept tests
    run_test_uart_peripheral_valid_mock();
    run_test_uart_peripheral_invalid_mock();
    run_test_uart_peripheral_not_basic_type();

    // Run SPI concept tests
    run_test_spi_peripheral_valid_mock();
    run_test_spi_peripheral_not_basic_type();

    // Run Timer concept tests
    run_test_timer_peripheral_valid_mock();
    run_test_timer_peripheral_not_basic_type();

    // Run ADC concept tests
    run_test_adc_peripheral_valid_mock();
    run_test_adc_peripheral_not_basic_type();

    // Run PWM concept tests
    run_test_pwm_peripheral_valid_mock();
    run_test_pwm_peripheral_not_basic_type();

    // Run capability concept tests
    run_test_interrupt_capable_valid_mock();
    run_test_interrupt_capable_not_basic_type();
    run_test_dma_capable_valid_mock();
    run_test_dma_capable_not_basic_type();

    // Run function tests
    run_test_concept_constrained_gpio_function();
    run_test_concept_constrained_uart_function();
    run_test_compile_time_concept_checks();

    // Print summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << tests_run << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
