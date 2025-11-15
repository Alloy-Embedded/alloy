/**
 * @file test_pin_signal_connection.cpp
 * @brief Unit tests for Pin→Signal Connection API (Phase 3.2)
 *
 * Tests the connection API for validating pin-to-signal connections with
 * helpful error messages and suggestions.
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
// Pin Type Aliases
// =============================================================================

using PinD4 = GpioPin<PIOD_BASE, 4>;  // USART0 RX
using PinA3 = GpioPin<PIOA_BASE, 3>;  // TWI0 SDA
using PinA0 = GpioPin<PIOA_BASE, 0>;  // PWM (not USART)

// =============================================================================
// validate_connection() Tests
// =============================================================================

TEST(validate_connection_valid_pin) {
    // PD4 is valid for USART0 RX
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PD4);
    ASSERT(conn.is_valid() == true);
    ASSERT(conn.get_pin() == PinId::PD4);
}

TEST(validate_connection_invalid_pin) {
    // PA0 is NOT valid for USART0 RX
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PA0);
    ASSERT(conn.is_valid() == false);
    ASSERT(conn.get_pin() == PinId::PA0);
    ASSERT(conn.error() != nullptr);
}

TEST(validate_connection_has_error_message) {
    // Invalid connection should have error message
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PA0);
    ASSERT(!conn.is_valid());
    ASSERT(std::string(conn.error()).length() > 0);
}

TEST(validate_connection_has_suggestion) {
    // Invalid connection should have suggestion
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PA0);
    ASSERT(!conn.is_valid());
    ASSERT(std::string(conn.hint()).length() > 0);
}

// =============================================================================
// connect<Signal, Pin>() Tests
// =============================================================================

TEST(connect_valid_pin_to_signal) {
    // Connect PD4 to USART0 RX (valid)
    constexpr auto conn = connect<USART0RXSignal, PinD4>();
    ASSERT(conn.is_valid() == true);
}

TEST(connect_invalid_pin_to_signal) {
    // Connect PA0 to USART0 RX (invalid)
    constexpr auto conn = connect<USART0RXSignal, PinA0>();
    ASSERT(conn.is_valid() == false);
}

TEST(connect_twi_signal) {
    // Connect PA3 to TWI0 SDA (valid)
    constexpr auto conn = connect<TWI0SDASignal, PinA3>();
    ASSERT(conn.is_valid() == true);
}

TEST(connect_wrong_twi_pin) {
    // Connect PD4 to TWI0 SDA (invalid - PD4 is for USART)
    constexpr auto conn = connect<TWI0SDASignal, PinD4>();
    ASSERT(conn.is_valid() == false);
}

// =============================================================================
// get_compatible_pins() Tests
// =============================================================================

TEST(get_compatible_pins_usart0_rx) {
    // Get compatible pins for USART0 RX
    constexpr auto pins = get_compatible_pins<USART0RXSignal>();
    ASSERT(pins.size() > 0);
    // Should include PD4
    bool found_pd4 = false;
    for (const auto& pin : pins) {
        if (pin == PinId::PD4) {
            found_pd4 = true;
            break;
        }
    }
    ASSERT(found_pd4 == true);
}

TEST(get_compatible_pins_twi0_sda) {
    // Get compatible pins for TWI0 SDA
    constexpr auto pins = get_compatible_pins<TWI0SDASignal>();
    ASSERT(pins.size() > 0);
    // Should include PA3
    bool found_pa3 = false;
    for (const auto& pin : pins) {
        if (pin == PinId::PA3) {
            found_pa3 = true;
            break;
        }
    }
    ASSERT(found_pa3 == true);
}

// =============================================================================
// Compile-Time Validation Tests
// =============================================================================

// These use static_assert to ensure compile-time validation works
static_assert(validate_connection<USART0RXSignal>(PinId::PD4).is_valid(),
             "PD4 must be valid for USART0 RX");
static_assert(!validate_connection<USART0RXSignal>(PinId::PA0).is_valid(),
             "PA0 must NOT be valid for USART0 RX");

TEST(compile_time_validation_works) {
    // If we reach here, all static_asserts passed
    ASSERT(true);
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(validate_connection_is_constexpr) {
    // Verify validate_connection can be evaluated at compile-time
    constexpr auto conn1 = validate_connection<USART0RXSignal>(PinId::PD4);
    constexpr auto conn2 = validate_connection<TWI0SDASignal>(PinId::PA3);

    ASSERT(conn1.is_valid() == true);
    ASSERT(conn2.is_valid() == true);
}

TEST(connect_is_constexpr) {
    // Verify connect can be evaluated at compile-time
    constexpr auto conn1 = connect<USART0RXSignal, PinD4>();
    constexpr auto conn2 = connect<TWI0SDASignal, PinA3>();

    ASSERT(conn1.is_valid() == true);
    ASSERT(conn2.is_valid() == true);
}

TEST(get_compatible_pins_is_constexpr) {
    // Verify get_compatible_pins can be evaluated at compile-time
    constexpr auto pins = get_compatible_pins<USART0RXSignal>();
    ASSERT(pins.size() > 0);
}

// =============================================================================
// Error Message Quality Tests
// =============================================================================

TEST(error_message_is_descriptive) {
    // Error message should explain what's wrong
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PA0);
    std::string error(conn.error());

    // Should mention incompatibility
    ASSERT(error.find("not") != std::string::npos ||
          error.find("does") != std::string::npos);
}

TEST(suggestion_is_helpful) {
    // Suggestion should guide user to fix
    constexpr auto conn = validate_connection<USART0RXSignal>(PinId::PA0);
    std::string hint(conn.hint());

    // Should mention checking or tables
    ASSERT(hint.find("Check") != std::string::npos ||
          hint.find("table") != std::string::npos);
}

// =============================================================================
// Multiple Signal Tests
// =============================================================================

TEST(pin_can_connect_to_different_signals) {
    // PA3 supports TWI0 SDA
    constexpr auto twi_conn = connect<TWI0SDASignal, PinA3>();
    ASSERT(twi_conn.is_valid() == true);

    // But PA3 should NOT support USART0 RX
    constexpr auto usart_conn = connect<USART0RXSignal, PinA3>();
    ASSERT(usart_conn.is_valid() == false);
}

TEST(different_pins_same_signal) {
    // Both should be evaluated independently
    constexpr auto conn1 = connect<USART0RXSignal, PinD4>();  // Valid
    constexpr auto conn2 = connect<USART0RXSignal, PinA0>();  // Invalid

    ASSERT(conn1.is_valid() == true);
    ASSERT(conn2.is_valid() == false);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Pin→Signal Connection API Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // validate_connection tests
    run_test_validate_connection_valid_pin();
    run_test_validate_connection_invalid_pin();
    run_test_validate_connection_has_error_message();
    run_test_validate_connection_has_suggestion();

    // connect tests
    run_test_connect_valid_pin_to_signal();
    run_test_connect_invalid_pin_to_signal();
    run_test_connect_twi_signal();
    run_test_connect_wrong_twi_pin();

    // get_compatible_pins tests
    run_test_get_compatible_pins_usart0_rx();
    run_test_get_compatible_pins_twi0_sda();

    // Compile-time validation
    run_test_compile_time_validation_works();

    // Constexpr tests
    run_test_validate_connection_is_constexpr();
    run_test_connect_is_constexpr();
    run_test_get_compatible_pins_is_constexpr();

    // Error message quality
    run_test_error_message_is_descriptive();
    run_test_suggestion_is_helpful();

    // Multiple signal tests
    run_test_pin_can_connect_to_different_signals();
    run_test_different_pins_same_signal();

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
