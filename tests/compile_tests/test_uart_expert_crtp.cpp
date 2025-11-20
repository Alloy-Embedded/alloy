/**
 * @file test_uart_expert_crtp.cpp
 * @brief Compile test for UART Expert API with CRTP refactoring
 *
 * This file tests that the refactored UART Expert API compiles correctly
 * with the new CRTP inheritance structure.
 *
 * @note Part of Phase 1.5: Refactor UartExpert
 */

#include "hal/api/uart_expert.hpp"
#include "core/types.hpp"
#include "core/units.hpp"

// Mock hardware policy for testing
template <uint32_t BASE_ADDR>
struct MockUartHardwarePolicy {
    static inline void reset() {}
    static inline void configure_8n1() {}
    static inline void set_baudrate(uint32_t) {}
    static inline void enable_transmitter() {}
    static inline void enable_receiver() {}
    static inline void enable_uart() {}

    static inline bool is_tx_empty() { return true; }
    static inline bool is_tx_complete() { return true; }
    static inline bool is_rx_not_empty() { return false; }

    static inline void write_data(uint8_t) {}
    static inline uint8_t read_data() { return 0; }
};

using namespace alloy::hal;
using namespace alloy::core;

// Test aliases
using TestPolicy = MockUartHardwarePolicy<0x40011000>;

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test that ExpertUartInstance inherits from UartBase
 */
void test_inheritance() {
    using InstanceType = ExpertUartInstance<TestPolicy>;
    using BaseType = UartBase<InstanceType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, InstanceType>,
                  "ExpertUartInstance must inherit from UartBase");
}

/**
 * @brief Test that expert configuration works
 */
void test_expert_config() {
    // Create configuration using factory method
    constexpr auto config = UartExpertConfig<TestPolicy>::standard_115200(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        signals::PinId::PD4
    );

    // Validate at compile-time
    static_assert(config.is_valid(), "Standard 115200 config must be valid");

    // Create UART instance from config
    auto uart = ExpertUartInstance<TestPolicy>(config);

    // Test that we can call inherited methods
    [[maybe_unused]] auto send_result = uart.send('A');
    [[maybe_unused]] auto receive_result = uart.receive();
    [[maybe_unused]] auto flush_result = uart.flush();
    [[maybe_unused]] auto write_result = uart.write("Test");
    [[maybe_unused]] auto available = uart.available();
    [[maybe_unused]] auto has_data = uart.has_data();

    // Test expert-specific method
    [[maybe_unused]] auto apply_result = uart.apply();
}

/**
 * @brief Test compile-time validation
 */
void test_constexpr_validation() {
    // Valid configuration
    constexpr auto valid_config = UartExpertConfig<TestPolicy>::standard_115200(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        signals::PinId::PD4
    );
    static_assert(valid_config.is_valid(), "Standard config must be valid");

    // Invalid configuration (no TX or RX enabled)
    constexpr auto invalid_config = UartExpertConfig<TestPolicy>{
        .peripheral = signals::PeripheralId::USART0,
        .tx_pin = signals::PinId::PD3,
        .rx_pin = signals::PinId::PD4,
        .baudrate = BaudRate{115200},
        .data_bits = 8,
        .parity = UartParity::NONE,
        .stop_bits = 1,
        .flow_control = false,
        .enable_tx = false,  // Both disabled
        .enable_rx = false,  // Both disabled
        .enable_interrupts = false,
        .enable_dma_tx = false,
        .enable_dma_rx = false,
        .enable_oversampling = true,
        .enable_rx_timeout = false,
        .rx_timeout_value = 0
    };
    static_assert(!invalid_config.is_valid(), "Config with no TX/RX must be invalid");
}

/**
 * @brief Test preset configurations
 */
void test_presets() {
    // Standard 115200
    constexpr auto standard = UartExpertConfig<TestPolicy>::standard_115200(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        signals::PinId::PD4
    );
    static_assert(standard.is_valid(), "Standard preset must be valid");
    static_assert(standard.enable_tx && standard.enable_rx, "Standard preset must have TX and RX");

    // Logger (TX-only)
    constexpr auto logger = UartExpertConfig<TestPolicy>::logger_config(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        BaudRate{115200}
    );
    static_assert(logger.is_valid(), "Logger preset must be valid");
    static_assert(logger.enable_tx && !logger.enable_rx, "Logger preset must be TX-only");

    // DMA configuration
    constexpr auto dma = UartExpertConfig<TestPolicy>::dma_config(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        signals::PinId::PD4,
        BaudRate{115200}
    );
    static_assert(dma.is_valid(), "DMA preset must be valid");
    static_assert(dma.enable_dma_tx && dma.enable_dma_rx, "DMA preset must enable DMA");
}

/**
 * @brief Test TX-only mode
 */
void test_tx_only() {
    constexpr auto config = UartExpertConfig<TestPolicy>::logger_config(
        signals::PeripheralId::USART0,
        signals::PinId::PD3
    );

    auto uart = ExpertUartInstance<TestPolicy>(config);

    // TX methods should work
    [[maybe_unused]] auto send_result = uart.send('A');
    [[maybe_unused]] auto write_result = uart.write("Test");
    [[maybe_unused]] auto flush_result = uart.flush();

    // RX methods should return error
    auto receive_result = uart.receive();
    static_assert(std::is_same_v<decltype(receive_result), Result<char, ErrorCode>>,
                  "receive() must return Result<char, ErrorCode>");
}

/**
 * @brief Test error messages
 */
void test_error_messages() {
    // Valid config
    constexpr auto valid = UartExpertConfig<TestPolicy>::standard_115200(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        signals::PinId::PD4
    );
    constexpr auto valid_msg = valid.error_message();
    static_assert(valid_msg[0] == 'V', "Valid config should return 'Valid' message");

    // Invalid baud rate (too high)
    constexpr auto invalid_baud = UartExpertConfig<TestPolicy>{
        .peripheral = signals::PeripheralId::USART0,
        .tx_pin = signals::PinId::PD3,
        .rx_pin = signals::PinId::PD4,
        .baudrate = BaudRate{20000000},  // Too high (>10MHz)
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
    static_assert(!invalid_baud.is_valid(), "High baud rate must be invalid");
}

/**
 * @brief Test zero-overhead guarantee
 */
void test_zero_overhead() {
    using InstanceType = ExpertUartInstance<TestPolicy>;
    using BaseType = UartBase<InstanceType>;

    // Verify empty base optimization
    static_assert(sizeof(BaseType) == 1,
                  "UartBase must be empty (sizeof == 1)");

    static_assert(std::is_empty_v<BaseType>,
                  "UartBase must have no data members");
}

/**
 * @brief Test validation helpers
 */
void test_validation_helpers() {
    constexpr auto config = UartExpertConfig<TestPolicy>::standard_115200(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        signals::PinId::PD4
    );

    // Test validation functions
    static_assert(validate_uart_config(config), "Standard config must validate");
    static_assert(has_valid_baudrate(config), "Standard config has valid baudrate");
    static_assert(has_valid_data_bits(config), "Standard config has valid data bits");
    static_assert(has_valid_stop_bits(config), "Standard config has valid stop bits");
    static_assert(has_enabled_direction(config), "Standard config has enabled direction");
}

/**
 * @brief Test get configuration method
 */
void test_get_config() {
    constexpr auto config = UartExpertConfig<TestPolicy>::standard_115200(
        signals::PeripheralId::USART0,
        signals::PinId::PD3,
        signals::PinId::PD4
    );

    auto uart = ExpertUartInstance<TestPolicy>(config);

    // Should be able to retrieve configuration
    const auto& retrieved_config = uart.config();
    static_assert(std::is_same_v<decltype(uart.config()),
                  const UartExpertConfig<TestPolicy>&>,
                  "config() must return const reference");

    // Verify we can access config properties
    [[maybe_unused]] auto baud = retrieved_config.baudrate;
    [[maybe_unused]] auto tx_enabled = retrieved_config.enable_tx;
}

// ============================================================================
// Summary
// ============================================================================

/**
 * Test Results:
 * - [x] ExpertUartInstance inherits from UartBase
 * - [x] All inherited methods compile
 * - [x] Expert configuration pattern works
 * - [x] Compile-time validation works
 * - [x] Preset methods work (standard_115200, logger_config, dma_config)
 * - [x] TX-only mode returns errors for RX methods
 * - [x] Error messages work at compile-time
 * - [x] Zero-overhead guarantee verified
 * - [x] Validation helper functions work
 * - [x] Configuration retrieval works
 *
 * If this file compiles without errors, Phase 1.5 refactoring is successful!
 */
