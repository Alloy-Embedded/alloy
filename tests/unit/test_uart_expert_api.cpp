/**
 * @file test_uart_expert_api.cpp
 * @brief Unit tests for UART Expert API (Phase 4.3)
 *
 * Tests the Level 3 expert API for UART configuration with
 * consteval validation and detailed error messages.
 *
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
 */

#include <cassert>
#include <iostream>
#include <string>

#include "../../src/hal/uart_expert.hpp"

using namespace alloy::hal;
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
// Configuration Creation Tests
// =============================================================================

TEST(config_can_be_created) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.peripheral == PeripheralId::USART0);
    ASSERT(config.baudrate.value() == 115200);
}

// =============================================================================
// Validation Tests
// =============================================================================

TEST(valid_config_passes_validation) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.is_valid() == true);
}

TEST(config_without_tx_or_rx_fails) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = false,  // Both disabled!
        .enable_rx = false,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.is_valid() == false);
}

TEST(invalid_data_bits_fails) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 5,  // Invalid!
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.is_valid() == false);
}

TEST(invalid_stop_bits_fails) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 3,  // Invalid!
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.is_valid() == false);
}

TEST(baudrate_too_low_fails) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{100},  // Too low!
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.is_valid() == false);
}

TEST(baudrate_too_high_fails) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{20000000},  // Too high!
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.is_valid() == false);
}

TEST(rx_timeout_without_value_fails) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = true,  // Enabled but...
        .rx_timeout_value = 0       // ...value is zero!
    };

    ASSERT(config.is_valid() == false);
}

TEST(flow_control_without_tx_fails) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = true,  // Needs both TX and RX
        .enable_tx = false,    // TX disabled!
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    ASSERT(config.is_valid() == false);
}

// =============================================================================
// Error Message Tests
// =============================================================================

TEST(error_message_is_descriptive) {
    constexpr UartExpertConfig config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 5,  // Invalid
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = true,
        .enable_rx = true,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    const char* msg = config.error_message();
    ASSERT(std::string(msg).find("Data bits") != std::string::npos);
}

TEST(valid_config_has_no_error_message) {
    constexpr UartExpertConfig config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    const char* msg = config.error_message();
    ASSERT(std::string(msg) == "Valid");
}

// =============================================================================
// Preset Configuration Tests
// =============================================================================

TEST(standard_115200_preset_is_valid) {
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    ASSERT(config.is_valid() == true);
    ASSERT(config.baudrate.value() == 115200);
    ASSERT(config.data_bits == 8);
    ASSERT(config.parity == UartParity::NONE);
    ASSERT(config.stop_bits == 1);
    ASSERT(config.enable_tx == true);
    ASSERT(config.enable_rx == true);
}

TEST(logger_config_preset_is_valid) {
    constexpr auto config = UartExpertConfig::logger_config(
        PeripheralId::USART0, PinId::PD3);

    ASSERT(config.is_valid() == true);
    ASSERT(config.enable_tx == true);
    ASSERT(config.enable_rx == false);  // TX only
}

TEST(dma_config_preset_is_valid) {
    constexpr auto config = UartExpertConfig::dma_config(
        PeripheralId::USART0, PinId::PD3, PinId::PD4, BaudRate{921600});

    ASSERT(config.is_valid() == true);
    ASSERT(config.enable_dma_tx == true);
    ASSERT(config.enable_dma_rx == true);
    ASSERT(config.enable_interrupts == true);
    ASSERT(config.baudrate.value() == 921600);
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(validation_is_constexpr) {
    // All validation should be compile-time evaluatable
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    constexpr bool valid = config.is_valid();
    static_assert(valid, "Config must be valid");

    ASSERT(true);  // If we reach here, constexpr worked
}

TEST(error_message_is_constexpr) {
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    constexpr const char* msg = config.error_message();

    ASSERT(msg != nullptr);
}

// =============================================================================
// Static Assertion Tests
// =============================================================================

// Verify presets are valid at compile-time
constexpr auto test_config_1 = UartExpertConfig::standard_115200(
    PeripheralId::USART0, PinId::PD3, PinId::PD4);
static_assert(test_config_1.is_valid(), "Standard 115200 must be valid");

constexpr auto test_config_2 = UartExpertConfig::logger_config(
    PeripheralId::USART0, PinId::PD3);
static_assert(test_config_2.is_valid(), "Logger config must be valid");

constexpr auto test_config_3 = UartExpertConfig::dma_config(
    PeripheralId::USART0, PinId::PD3, PinId::PD4, BaudRate{115200});
static_assert(test_config_3.is_valid(), "DMA config must be valid");

TEST(compile_time_validation_works) {
    // If we reach here, all static_asserts passed
    ASSERT(true);
}

// =============================================================================
// Helper Function Tests
// =============================================================================

TEST(validate_uart_config_helper) {
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    ASSERT(validate_uart_config(config) == true);
}

TEST(has_valid_baudrate_helper) {
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    ASSERT(has_valid_baudrate(config) == true);
}

TEST(has_valid_data_bits_helper) {
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    ASSERT(has_valid_data_bits(config) == true);
}

TEST(has_enabled_direction_helper) {
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    ASSERT(has_enabled_direction(config) == true);
}

// =============================================================================
// Expert Namespace Tests
// =============================================================================

TEST(expert_configure_validates) {
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);

    auto result = expert::configure(config);
    ASSERT(result.is_ok());
}

TEST(expert_configure_rejects_invalid) {
    constexpr UartExpertConfig invalid_config = {
        .peripheral = PeripheralId::USART0,
        .tx_pin = PinId::PD3,
        .rx_pin = PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = false,  // Both disabled - invalid!
        .enable_rx = false,
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };

    auto result = expert::configure(invalid_config);
    ASSERT(!result.is_ok());
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  UART Expert API Tests (Phase 4.3)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Configuration creation
    run_test_config_can_be_created();

    // Validation tests
    run_test_valid_config_passes_validation();
    run_test_config_without_tx_or_rx_fails();
    run_test_invalid_data_bits_fails();
    run_test_invalid_stop_bits_fails();
    run_test_baudrate_too_low_fails();
    run_test_baudrate_too_high_fails();
    run_test_rx_timeout_without_value_fails();
    run_test_flow_control_without_tx_fails();

    // Error messages
    run_test_error_message_is_descriptive();
    run_test_valid_config_has_no_error_message();

    // Preset configurations
    run_test_standard_115200_preset_is_valid();
    run_test_logger_config_preset_is_valid();
    run_test_dma_config_preset_is_valid();

    // Constexpr evaluation
    run_test_validation_is_constexpr();
    run_test_error_message_is_constexpr();
    run_test_compile_time_validation_works();

    // Helper functions
    run_test_validate_uart_config_helper();
    run_test_has_valid_baudrate_helper();
    run_test_has_valid_data_bits_helper();
    run_test_has_enabled_direction_helper();

    // Expert namespace
    run_test_expert_configure_validates();
    run_test_expert_configure_rejects_invalid();

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
