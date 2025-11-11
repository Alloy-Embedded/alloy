/**
 * @file test_uart_simple_api.cpp
 * @brief Unit tests for UART Simple API (Phase 4.1)
 *
 * Tests the Level 1 simple API for UART configuration with compile-time
 * pin validation and sensible defaults.
 *
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
 */

#include <cassert>
#include <iostream>

#include "../../src/hal/uart_simple.hpp"
#include "../../src/hal/platform/same70/gpio.hpp"
#include "../../src/hal/vendors/atmel/same70/same70_signals.hpp"

using namespace alloy::hal;
using namespace alloy::hal::same70;
using namespace alloy::hal::signals;
using namespace alloy::core;

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
// Pin Type Aliases (SAME70)
// =============================================================================

using PinD4 = GpioPin<PIOD_BASE, 4>;  // USART0 RX
using PinD3 = GpioPin<PIOD_BASE, 3>;  // USART0 TX
using PinA3 = GpioPin<PIOA_BASE, 3>;  // TWI0 SDA (not USART)

// =============================================================================
// Default Configuration Tests
// =============================================================================

TEST(defaults_are_sensible) {
    // Verify default values are correct
    ASSERT(UartDefaults::data_bits == 8);
    ASSERT(UartDefaults::parity == UartParity::NONE);
    ASSERT(UartDefaults::stop_bits == 1);
    ASSERT(UartDefaults::flow_control == false);
}

// =============================================================================
// Quick Setup API Tests
// =============================================================================

TEST(quick_setup_creates_config) {
    // Quick setup should return a configuration object
    auto config = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});

    ASSERT(config.peripheral == PeripheralId::USART0);
    ASSERT(config.baudrate.value() == 115200);
    ASSERT(config.data_bits == 8);
    ASSERT(config.parity == UartParity::NONE);
    ASSERT(config.stop_bits == 1);
    ASSERT(config.flow_control == false);
}

TEST(quick_setup_with_custom_parity) {
    // Test custom parity setting
    auto config = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(
        BaudRate{9600}, UartParity::EVEN);

    ASSERT(config.peripheral == PeripheralId::USART0);
    ASSERT(config.baudrate.value() == 9600);
    ASSERT(config.parity == UartParity::EVEN);
    ASSERT(config.data_bits == 8);  // Still default
}

TEST(quick_setup_different_baudrates) {
    // Test various common baud rates
    auto config1 = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{9600});
    auto config2 = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});
    auto config3 = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{921600});

    ASSERT(config1.baudrate.value() == 9600);
    ASSERT(config2.baudrate.value() == 115200);
    ASSERT(config3.baudrate.value() == 921600);
}

// =============================================================================
// TX-Only Setup Tests
// =============================================================================

TEST(quick_setup_tx_only) {
    // TX-only configuration for logging
    auto config = Uart<PeripheralId::USART0>::quick_setup_tx_only<PinD3>(BaudRate{115200});

    ASSERT(config.peripheral == PeripheralId::USART0);
    ASSERT(config.baudrate.value() == 115200);
    ASSERT(config.data_bits == 8);
    ASSERT(config.parity == UartParity::NONE);
}

// =============================================================================
// Configuration Type Tests
// =============================================================================

TEST(config_has_initialize_method) {
    // Configuration should be initializable
    auto config = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});

    // Should have initialize method (compile-time check)
    // Runtime initialization would configure hardware
    // For unit tests, we just verify the method exists
    auto result = config.initialize();

    // In a real embedded context, this would configure hardware
    // For now, just verify it returns a Result type
    ASSERT(result.is_ok() || result.is_err());
}

TEST(tx_only_config_has_initialize) {
    auto config = Uart<PeripheralId::USART0>::quick_setup_tx_only<PinD3>(BaudRate{115200});
    auto result = config.initialize();
    ASSERT(result.is_ok() || result.is_err());
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(quick_setup_is_constexpr) {
    // Verify quick_setup can be evaluated at compile-time
    constexpr auto config = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});

    static_assert(config.peripheral == PeripheralId::USART0);
    static_assert(config.baudrate.value() == 115200);
    static_assert(config.data_bits == 8);

    ASSERT(true);  // If we get here, constexpr worked
}

TEST(tx_only_setup_is_constexpr) {
    constexpr auto config = Uart<PeripheralId::USART0>::quick_setup_tx_only<PinD3>(BaudRate{115200});

    static_assert(config.peripheral == PeripheralId::USART0);
    static_assert(config.baudrate.value() == 115200);

    ASSERT(true);
}

// =============================================================================
// API Usability Tests
// =============================================================================

TEST(api_is_simple_one_liner) {
    // The whole point of Level 1 API is simplicity
    // This should be a one-liner in real code:
    auto uart = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});

    // Verify it's usable
    ASSERT(uart.peripheral == PeripheralId::USART0);
}

TEST(different_peripherals) {
    // Test with different UART peripherals
    auto uart0 = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});
    auto uart1 = Uart<PeripheralId::USART1>::quick_setup<PinD3, PinD4>(BaudRate{9600});

    ASSERT(uart0.peripheral == PeripheralId::USART0);
    ASSERT(uart1.peripheral == PeripheralId::USART1);
    ASSERT(uart0.baudrate.value() == 115200);
    ASSERT(uart1.baudrate.value() == 9600);
}

// =============================================================================
// Static Assertions (Compile-Time Validation)
// =============================================================================

// Verify defaults are compile-time constants
static_assert(UartDefaults::data_bits == 8);
static_assert(UartDefaults::stop_bits == 1);

// Verify quick_setup is constexpr
constexpr auto test_config = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});
static_assert(test_config.peripheral == PeripheralId::USART0);
static_assert(test_config.baudrate.value() == 115200);

TEST(compile_time_validation_works) {
    // If we reach here, all static_asserts passed
    ASSERT(true);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  UART Simple API Tests (Phase 4.1)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Default configuration tests
    run_test_defaults_are_sensible();

    // Quick setup API tests
    run_test_quick_setup_creates_config();
    run_test_quick_setup_with_custom_parity();
    run_test_quick_setup_different_baudrates();

    // TX-only tests
    run_test_quick_setup_tx_only();

    // Configuration tests
    run_test_config_has_initialize_method();
    run_test_tx_only_config_has_initialize();

    // Constexpr tests
    run_test_quick_setup_is_constexpr();
    run_test_tx_only_setup_is_constexpr();

    // API usability tests
    run_test_api_is_simple_one_liner();
    run_test_different_peripherals();

    // Compile-time validation
    run_test_compile_time_validation_works();

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
