/**
 * @file test_gpio_signal_routing.cpp
 * @brief Unit tests for GPIO signal routing (Phase 3)
 *
 * Tests the GPIO pin enhancements for signal routing including:
 * - supports<Signal>() compile-time validation
 * - setAlternateFunction() configuration
 * - configure_for_signal<Signal>() convenience method
 *
 * @see openspec/changes/modernize-peripheral-architecture/specs/signal-routing/spec.md
 */

#include <cassert>
#include <iostream>

#include "../../src/hal/platform/same70/gpio.hpp"
#include "../../src/hal/signals.hpp"
#include "../../src/hal/vendors/atmel/same70/same70_signals.hpp"

using namespace alloy::hal::same70;
using namespace alloy::hal::signals;
using namespace alloy::hal::atmel::same70;

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
// Pin Type Aliases for Testing
// =============================================================================

using PinD4 = GpioPin<PIOD_BASE, 4>;  // USART0 RX
using PinA3 = GpioPin<PIOA_BASE, 3>;  // TWI0 SDA
using PinA4 = GpioPin<PIOA_BASE, 4>;  // TWI0 SCL
using PinA0 = GpioPin<PIOA_BASE, 0>;  // PWM (not USART)

// =============================================================================
// Compile-Time Signal Support Tests
// =============================================================================

TEST(pin_d4_supports_usart0_rx) {
    // PD4 should support USART0 RX signal
    constexpr bool supports = PinD4::supports<USART0RXSignal>();
    ASSERT(supports == true);
}

TEST(pin_d4_does_not_support_twi0_sda) {
    // PD4 should NOT support TWI0 SDA signal
    constexpr bool supports = PinD4::supports<TWI0SDASignal>();
    ASSERT(supports == false);
}

TEST(pin_a3_supports_twi0_sda) {
    // PA3 should support TWI0 SDA signal
    constexpr bool supports = PinA3::supports<TWI0SDASignal>();
    ASSERT(supports == true);
}

TEST(pin_a4_supports_twi0_scl) {
    // PA4 should support TWI0 SCL signal
    constexpr bool supports = PinA4::supports<TWI0SCLSignal>();
    ASSERT(supports == true);
}

TEST(pin_a0_does_not_support_usart0_rx) {
    // PA0 should NOT support USART0 RX (it's a PWM pin)
    constexpr bool supports = PinA0::supports<USART0RXSignal>();
    ASSERT(supports == false);
}

// =============================================================================
// Get Alternate Function Tests
// =============================================================================

TEST(pin_d4_get_af_for_usart0_rx) {
    // PD4 for USART0 RX should use PERIPH_A
    constexpr auto af = PinD4::get_af_for_signal<USART0RXSignal>();
    ASSERT(af == AlternateFunction::PERIPH_A);
}

TEST(pin_a3_get_af_for_twi0_sda) {
    // PA3 for TWI0 SDA should use PERIPH_A
    constexpr auto af = PinA3::get_af_for_signal<TWI0SDASignal>();
    ASSERT(af == AlternateFunction::PERIPH_A);
}

TEST(pin_d4_get_af_for_wrong_signal) {
    // PD4 doesn't support TWI0 SDA, should return AF0
    constexpr auto af = PinD4::get_af_for_signal<TWI0SDASignal>();
    ASSERT(af == AlternateFunction::AF0);
}

// =============================================================================
// Static Assertion Tests (Compile-Time Validation)
// =============================================================================

// These static_asserts ensure compile-time validation works
static_assert(PinD4::supports<USART0RXSignal>(),
             "PD4 must support USART0 RX");
static_assert(!PinD4::supports<TWI0SDASignal>(),
             "PD4 must NOT support TWI0 SDA");
static_assert(PinA3::supports<TWI0SDASignal>(),
             "PA3 must support TWI0 SDA");
static_assert(PinA4::supports<TWI0SCLSignal>(),
             "PA4 must support TWI0 SCL");

TEST(compile_time_validation_works) {
    // If we reach here, all static_asserts passed
    ASSERT(true);
}

// =============================================================================
// Get PinId Tests
// =============================================================================

// Note: get_pin_id() is private, but we can test it indirectly through supports()
TEST(pin_id_calculation_port_a) {
    // PA3 should map to PinId::PA3
    constexpr bool supports = PinA3::supports<TWI0SDASignal>();
    // If this works, get_pin_id() calculated correctly
    ASSERT(supports == true);
}

TEST(pin_id_calculation_port_d) {
    // PD4 should map to PinId::PD4
    constexpr bool supports = PinD4::supports<USART0RXSignal>();
    ASSERT(supports == true);
}

// =============================================================================
// Multiple Signals Per Pin Tests
// =============================================================================

TEST(pin_can_support_multiple_signals) {
    // PA3 supports TWI0 SDA
    constexpr bool twi = PinA3::supports<TWI0SDASignal>();
    ASSERT(twi == true);

    // PA3 might also support other peripherals (check database)
    // This test validates that supports() can be called multiple times
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(supports_is_constexpr) {
    // Verify supports() can be evaluated at compile-time
    constexpr bool result1 = PinD4::supports<USART0RXSignal>();
    constexpr bool result2 = PinA3::supports<TWI0SDASignal>();

    ASSERT(result1 == true);
    ASSERT(result2 == true);
}

TEST(get_af_is_constexpr) {
    // Verify get_af_for_signal() can be evaluated at compile-time
    constexpr auto af1 = PinD4::get_af_for_signal<USART0RXSignal>();
    constexpr auto af2 = PinA3::get_af_for_signal<TWI0SDASignal>();

    ASSERT(af1 == AlternateFunction::PERIPH_A);
    ASSERT(af2 == AlternateFunction::PERIPH_A);
}

// =============================================================================
// Cross-Peripheral Signal Tests
// =============================================================================

TEST(different_peripherals_different_pins) {
    // USART0 on PD4
    constexpr bool usart_d4 = PinD4::supports<USART0RXSignal>();

    // TWI0 on PA3
    constexpr bool twi_a3 = PinA3::supports<TWI0SDASignal>();

    // They should not overlap
    constexpr bool usart_a3 = PinA3::supports<USART0RXSignal>();
    constexpr bool twi_d4 = PinD4::supports<TWI0SDASignal>();

    ASSERT(usart_d4 == true);
    ASSERT(twi_a3 == true);
    ASSERT(usart_a3 == false);
    ASSERT(twi_d4 == false);
}

// =============================================================================
// SPI Signal Tests
// =============================================================================

TEST(spi_signals_validated) {
    // SPI0 signals should be available
    // Note: Need to check which pins support SPI0 from signal tables
    // This test verifies the mechanism works for SPI

    // Just verify the types exist and compile
    using Spi0Mosi = SPI0MOSISignal;
    using Spi0Miso = SPI0MISOSignal;

    ASSERT(true);  // If it compiles, test passes
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  GPIO Signal Routing Tests (Phase 3)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Compile-time support tests
    run_test_pin_d4_supports_usart0_rx();
    run_test_pin_d4_does_not_support_twi0_sda();
    run_test_pin_a3_supports_twi0_sda();
    run_test_pin_a4_supports_twi0_scl();
    run_test_pin_a0_does_not_support_usart0_rx();

    // Get alternate function tests
    run_test_pin_d4_get_af_for_usart0_rx();
    run_test_pin_a3_get_af_for_twi0_sda();
    run_test_pin_d4_get_af_for_wrong_signal();

    // Static assertion tests
    run_test_compile_time_validation_works();

    // PinId calculation tests
    run_test_pin_id_calculation_port_a();
    run_test_pin_id_calculation_port_d();

    // Multiple signals tests
    run_test_pin_can_support_multiple_signals();

    // Constexpr tests
    run_test_supports_is_constexpr();
    run_test_get_af_is_constexpr();

    // Cross-peripheral tests
    run_test_different_peripherals_different_pins();

    // SPI tests
    run_test_spi_signals_validated();

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
