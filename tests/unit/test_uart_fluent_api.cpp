/**
 * @file test_uart_fluent_api.cpp
 * @brief Unit tests for UART Fluent API (Phase 4.2)
 *
 * Tests the Level 2 fluent builder API for UART configuration with
 * method chaining and incremental validation.
 *
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
 */

#include <cassert>
#include <iostream>

#include "../../src/hal/uart_fluent.hpp"
#include "../../src/hal/platform/same70/gpio.hpp"

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

// =============================================================================
// Builder Construction Tests
// =============================================================================

TEST(builder_default_construction) {
    // Builder should construct with defaults
    auto builder = UartBuilder<PeripheralId::USART0>();
    auto state = builder.get_state();

    ASSERT(state.has_tx_pin == false);
    ASSERT(state.has_rx_pin == false);
    ASSERT(state.has_baudrate == false);
}

// =============================================================================
// Pin Configuration Tests
// =============================================================================

TEST(with_tx_pin_sets_state) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<PinD3>();

    auto state = builder.get_state();
    ASSERT(state.has_tx_pin == true);
    ASSERT(state.has_rx_pin == false);
}

TEST(with_rx_pin_sets_state) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_rx_pin<PinD4>();

    auto state = builder.get_state();
    ASSERT(state.has_tx_pin == false);
    ASSERT(state.has_rx_pin == true);
}

TEST(with_pins_sets_both) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>();

    auto state = builder.get_state();
    ASSERT(state.has_tx_pin == true);
    ASSERT(state.has_rx_pin == true);
}

// =============================================================================
// Parameter Configuration Tests
// =============================================================================

TEST(baudrate_sets_state) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .baudrate(BaudRate{115200});

    auto state = builder.get_state();
    ASSERT(state.has_baudrate == true);
}

TEST(parity_sets_state) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .parity(UartParity::EVEN);

    auto state = builder.get_state();
    ASSERT(state.has_parity == true);
}

TEST(data_bits_sets_state) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .data_bits(8);

    auto state = builder.get_state();
    ASSERT(state.has_data_bits == true);
}

TEST(stop_bits_sets_state) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .stop_bits(1);

    auto state = builder.get_state();
    ASSERT(state.has_stop_bits == true);
}

// =============================================================================
// Method Chaining Tests
// =============================================================================

TEST(methods_chain_together) {
    // All methods should return reference for chaining
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<PinD3>()
        .with_rx_pin<PinD4>()
        .baudrate(BaudRate{115200})
        .parity(UartParity::EVEN)
        .data_bits(8)
        .stop_bits(1);

    auto state = builder.get_state();
    ASSERT(state.has_tx_pin == true);
    ASSERT(state.has_rx_pin == true);
    ASSERT(state.has_baudrate == true);
    ASSERT(state.has_parity == true);
    ASSERT(state.has_data_bits == true);
    ASSERT(state.has_stop_bits == true);
}

TEST(reads_like_natural_language) {
    // Fluent API should be self-documenting
    auto uart = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<PinD3>()
        .with_rx_pin<PinD4>()
        .baudrate(BaudRate{115200})
        .parity(UartParity::NONE)
        .data_bits(8)
        .stop_bits(1)
        .initialize();

    ASSERT(uart.is_ok());
}

// =============================================================================
// Preset Configuration Tests
// =============================================================================

TEST(standard_8n1_preset) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .standard_8n1();

    auto state = builder.get_state();
    ASSERT(state.has_data_bits == true);
    ASSERT(state.has_parity == true);
    ASSERT(state.has_stop_bits == true);
}

TEST(standard_8e1_preset) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .standard_8e1();

    auto state = builder.get_state();
    ASSERT(state.has_data_bits == true);
    ASSERT(state.has_parity == true);
    ASSERT(state.has_stop_bits == true);
}

TEST(standard_8o1_preset) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .standard_8o1();

    auto state = builder.get_state();
    ASSERT(state.has_data_bits == true);
    ASSERT(state.has_parity == true);
    ASSERT(state.has_stop_bits == true);
}

TEST(preset_with_other_params) {
    // Presets should chain with other parameters
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>()
        .baudrate(BaudRate{9600})
        .standard_8n1();

    auto state = builder.get_state();
    ASSERT(state.has_tx_pin == true);
    ASSERT(state.has_rx_pin == true);
    ASSERT(state.has_baudrate == true);
    ASSERT(state.has_data_bits == true);
}

// =============================================================================
// Validation Tests
// =============================================================================

TEST(validation_fails_without_baudrate) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>();

    auto result = builder.validate();
    ASSERT(!result.is_ok());
}

TEST(validation_fails_without_pins) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .baudrate(BaudRate{115200});

    auto result = builder.validate();
    ASSERT(!result.is_ok());
}

TEST(validation_succeeds_with_minimal_config) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<PinD3>()
        .baudrate(BaudRate{115200});

    auto result = builder.validate();
    ASSERT(result.is_ok());
}

TEST(validation_succeeds_with_full_config) {
    auto builder = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>()
        .baudrate(BaudRate{115200})
        .standard_8n1();

    auto result = builder.validate();
    ASSERT(result.is_ok());
}

// =============================================================================
// Initialization Tests
// =============================================================================

TEST(initialize_returns_config_when_valid) {
    auto result = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>()
        .baudrate(BaudRate{115200})
        .initialize();

    ASSERT(result.is_ok());

    auto config = result.unwrap();
    ASSERT(config.peripheral == PeripheralId::USART0);
    ASSERT(config.baudrate.value() == 115200);
    ASSERT(config.has_tx == true);
    ASSERT(config.has_rx == true);
}

TEST(initialize_fails_when_invalid) {
    auto result = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<PinD3>()  // No baudrate!
        .initialize();

    ASSERT(!result.is_ok());
}

TEST(initialize_tx_only_config) {
    auto result = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<PinD3>()
        .baudrate(BaudRate{115200})
        .initialize();

    ASSERT(result.is_ok());

    auto config = result.unwrap();
    ASSERT(config.has_tx == true);
    ASSERT(config.has_rx == false);
}

TEST(initialize_rx_only_config) {
    auto result = UartBuilder<PeripheralId::USART0>()
        .with_rx_pin<PinD4>()
        .baudrate(BaudRate{115200})
        .initialize();

    ASSERT(result.is_ok());

    auto config = result.unwrap();
    ASSERT(config.has_tx == false);
    ASSERT(config.has_rx == true);
}

// =============================================================================
// Configuration Result Tests
// =============================================================================

TEST(config_has_apply_method) {
    auto result = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>()
        .baudrate(BaudRate{115200})
        .initialize();

    ASSERT(result.is_ok());

    auto config = result.unwrap();
    auto apply_result = config.apply();
    ASSERT(apply_result.is_ok());
}

// =============================================================================
// Constexpr Tests
// =============================================================================

TEST(builder_state_is_constexpr) {
    // BuilderState operations should be constexpr
    constexpr BuilderState state{
        .has_tx_pin = true,
        .has_rx_pin = true,
        .has_baudrate = true,
        .has_parity = false,
        .has_data_bits = false,
        .has_stop_bits = false
    };

    static_assert(state.is_full_duplex_valid());
    static_assert(state.is_valid());

    ASSERT(true);  // If we reach here, constexpr worked
}

TEST(builder_methods_are_constexpr) {
    // Builder methods should be constexpr
    constexpr auto builder = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>()
        .baudrate(BaudRate{115200})
        .standard_8n1();

    constexpr auto state = builder.get_state();
    static_assert(state.has_tx_pin);
    static_assert(state.has_rx_pin);
    static_assert(state.has_baudrate);

    ASSERT(true);
}

// =============================================================================
// API Usability Tests
// =============================================================================

TEST(api_is_readable) {
    // Real-world usage example
    auto uart = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<PinD3>()
        .with_rx_pin<PinD4>()
        .baudrate(BaudRate{115200})
        .standard_8n1()
        .initialize();

    ASSERT(uart.is_ok());
}

TEST(different_configurations) {
    // Multiple different configurations
    auto uart1 = UartBuilder<PeripheralId::USART0>()
        .with_pins<PinD3, PinD4>()
        .baudrate(BaudRate{9600})
        .standard_8n1()
        .initialize();

    auto uart2 = UartBuilder<PeripheralId::USART1>()
        .with_tx_pin<PinD3>()
        .baudrate(BaudRate{115200})
        .standard_8e1()
        .initialize();

    ASSERT(uart1.is_ok());
    ASSERT(uart2.is_ok());

    ASSERT(uart1.unwrap().baudrate.value() == 9600);
    ASSERT(uart2.unwrap().baudrate.value() == 115200);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  UART Fluent API Tests (Phase 4.2)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Builder construction
    run_test_builder_default_construction();

    // Pin configuration
    run_test_with_tx_pin_sets_state();
    run_test_with_rx_pin_sets_state();
    run_test_with_pins_sets_both();

    // Parameter configuration
    run_test_baudrate_sets_state();
    run_test_parity_sets_state();
    run_test_data_bits_sets_state();
    run_test_stop_bits_sets_state();

    // Method chaining
    run_test_methods_chain_together();
    run_test_reads_like_natural_language();

    // Preset configurations
    run_test_standard_8n1_preset();
    run_test_standard_8e1_preset();
    run_test_standard_8o1_preset();
    run_test_preset_with_other_params();

    // Validation
    run_test_validation_fails_without_baudrate();
    run_test_validation_fails_without_pins();
    run_test_validation_succeeds_with_minimal_config();
    run_test_validation_succeeds_with_full_config();

    // Initialization
    run_test_initialize_returns_config_when_valid();
    run_test_initialize_fails_when_invalid();
    run_test_initialize_tx_only_config();
    run_test_initialize_rx_only_config();

    // Configuration result
    run_test_config_has_apply_method();

    // Constexpr
    run_test_builder_state_is_constexpr();
    run_test_builder_methods_are_constexpr();

    // API usability
    run_test_api_is_readable();
    run_test_different_configurations();

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
