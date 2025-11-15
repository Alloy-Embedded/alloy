/**
 * @file test_signal_tables.cpp
 * @brief Unit tests for generated signal routing tables
 *
 * Tests the auto-generated signal routing tables from Phase 2.
 * Validates that signal definitions compile and provide correct information.
 *
 * @see openspec/changes/modernize-peripheral-architecture/specs/signal-routing/spec.md
 */

#include <cassert>
#include <iostream>

#include "../../src/hal/signals.hpp"
#include "../../src/hal/vendors/atmel/same70/same70_signals.hpp"

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
// Signal Table Structure Tests
// =============================================================================

TEST(usart0_rx_signal_has_peripheral_id) {
    ASSERT(USART0RXSignal::peripheral == PeripheralId::USART0);
}

TEST(usart0_rx_signal_has_signal_type) {
    ASSERT(USART0RXSignal::type == SignalType::RX);
}

TEST(usart0_rx_signal_has_compatible_pins) {
    // USART0 RX on SAME70 should have at least one compatible pin
    ASSERT(USART0RXSignal::compatible_pins.size() > 0);
}

TEST(usart0_rx_signal_pin_has_alternate_function) {
    // First pin should have a valid alternate function
    auto pin = USART0RXSignal::compatible_pins[0];
    ASSERT(pin.pin == PinId::PD4);
    ASSERT(pin.af == AlternateFunction::PERIPH_A);
}

TEST(spi0_mosi_signal_structure) {
    ASSERT(SPI0MOSISignal::peripheral == PeripheralId::SPI0);
    ASSERT(SPI0MOSISignal::type == SignalType::MOSI);
    ASSERT(SPI0MOSISignal::compatible_pins.size() > 0);
}

TEST(spi0_miso_signal_structure) {
    ASSERT(SPI0MISOSignal::peripheral == PeripheralId::SPI0);
    ASSERT(SPI0MISOSignal::type == SignalType::MISO);
    ASSERT(SPI0MISOSignal::compatible_pins.size() > 0);
}

TEST(twi0_sda_signal_structure) {
    ASSERT(TWI0SDASignal::peripheral == PeripheralId::TWI0);
    ASSERT(TWI0SDASignal::type == SignalType::SDA);
    ASSERT(TWI0SDASignal::compatible_pins.size() > 0);
}

TEST(twi0_scl_signal_structure) {
    ASSERT(TWI0SCLSignal::peripheral == PeripheralId::TWI0);
    ASSERT(TWI0SCLSignal::type == SignalType::SCL);
    ASSERT(TWI0SCLSignal::compatible_pins.size() > 0);
}

// =============================================================================
// Signal Validation Helper Tests
// =============================================================================

TEST(pin_supports_signal_usart0_rx) {
    // PD4 should support USART0 RX
    constexpr bool supports = pin_supports_signal<USART0RXSignal>(PinId::PD4);
    ASSERT(supports == true);
}

TEST(pin_supports_signal_wrong_pin) {
    // PA0 should NOT support USART0 RX
    constexpr bool supports = pin_supports_signal<USART0RXSignal>(PinId::PA0);
    ASSERT(supports == false);
}

TEST(get_alternate_function_usart0_rx) {
    // PD4 for USART0 RX should use PERIPH_A
    constexpr auto af = get_alternate_function<USART0RXSignal>(PinId::PD4);
    ASSERT(af == AlternateFunction::PERIPH_A);
}

TEST(get_alternate_function_wrong_pin) {
    // PA0 doesn't support USART0 RX, should return AF0
    constexpr auto af = get_alternate_function<USART0RXSignal>(PinId::PA0);
    ASSERT(af == AlternateFunction::AF0);
}

// =============================================================================
// Compile-Time Validation Tests
// =============================================================================

TEST(validate_connection_usart0_rx_valid) {
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PD4);
    ASSERT(conn.is_valid() == true);
}

TEST(validate_connection_usart0_rx_invalid) {
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PA0);
    ASSERT(conn.is_valid() == false);
}

// =============================================================================
// Multiple Signal Tests
// =============================================================================

TEST(spi0_has_all_signals) {
    // SPI0 should have MOSI, MISO, CLK, CS signals
    static_assert(SPI0MOSISignal::peripheral == PeripheralId::SPI0);
    static_assert(SPI0MISOSignal::peripheral == PeripheralId::SPI0);
    static_assert(SPI0CLKSignal::peripheral == PeripheralId::SPI0);
    static_assert(SPI0CSSignal::peripheral == PeripheralId::SPI0);
    ASSERT(true);
}

TEST(twi0_has_scl_sda) {
    // TWI0 (I2C) should have SCL and SDA signals
    static_assert(TWI0SCLSignal::peripheral == PeripheralId::TWI0);
    static_assert(TWI0SDASignal::peripheral == PeripheralId::TWI0);
    ASSERT(true);
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(signal_definitions_are_constexpr) {
    // All signal data should be evaluatable at compile-time
    constexpr auto usart_periph = USART0RXSignal::peripheral;
    constexpr auto usart_type = USART0RXSignal::type;
    constexpr auto pin_count = USART0RXSignal::compatible_pins.size();

    ASSERT(usart_periph == PeripheralId::USART0);
    ASSERT(usart_type == SignalType::RX);
    ASSERT(pin_count > 0);
}

TEST(pin_compatibility_is_constexpr) {
    // Pin compatibility checks should be compile-time
    static_assert(pin_supports_signal<USART0RXSignal>(PinId::PD4));
    static_assert(!pin_supports_signal<USART0RXSignal>(PinId::PA0));
    ASSERT(true);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Signal Table Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Structure tests
    run_test_usart0_rx_signal_has_peripheral_id();
    run_test_usart0_rx_signal_has_signal_type();
    run_test_usart0_rx_signal_has_compatible_pins();
    run_test_usart0_rx_signal_pin_has_alternate_function();
    run_test_spi0_mosi_signal_structure();
    run_test_spi0_miso_signal_structure();
    run_test_twi0_sda_signal_structure();
    run_test_twi0_scl_signal_structure();

    // Validation helper tests
    run_test_pin_supports_signal_usart0_rx();
    run_test_pin_supports_signal_wrong_pin();
    run_test_get_alternate_function_usart0_rx();
    run_test_get_alternate_function_wrong_pin();
    run_test_validate_connection_usart0_rx_valid();
    run_test_validate_connection_usart0_rx_invalid();

    // Multiple signal tests
    run_test_spi0_has_all_signals();
    run_test_twi0_has_scl_sda();

    // Constexpr tests
    run_test_signal_definitions_are_constexpr();
    run_test_pin_compatibility_is_constexpr();

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
