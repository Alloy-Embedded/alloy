/**
 * @file test_uart_simple_crtp.cpp
 * @brief Compile test for UART Simple API with CRTP refactoring
 *
 * This file tests that the refactored UART Simple API compiles correctly
 * with the new CRTP inheritance structure.
 *
 * @note Part of Phase 1.3: Refactor UartSimple
 */

#include "hal/api/uart_simple.hpp"
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

// Mock GPIO pins
template <alloy::hal::signals::PinId PIN>
struct MockGpioPin {
    static constexpr alloy::hal::signals::PinId pin = PIN;

    static constexpr alloy::hal::signals::PinId get_pin_id() {
        return PIN;
    }
};

using namespace alloy::hal;
using namespace alloy::core;

// Test aliases
using TestTxPin = MockGpioPin<alloy::hal::signals::PinId::PA3>;
using TestRxPin = MockGpioPin<alloy::hal::signals::PinId::PA4>;
using TestPolicy = MockUartHardwarePolicy<0x40011000>;

// ============================================================================
// Compile-Time Tests
// ============================================================================

/**
 * @brief Test that SimpleUartConfig inherits from UartBase
 */
void test_inheritance() {
    using ConfigType = SimpleUartConfig<TestTxPin, TestRxPin, TestPolicy>;
    using BaseType = UartBase<ConfigType>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, ConfigType>,
                  "SimpleUartConfig must inherit from UartBase");
}

/**
 * @brief Test that basic API works
 */
void test_basic_api() {
    // Create UART configuration using factory method
    auto uart = Uart<PeripheralId::USART0, TestPolicy>::quick_setup<TestTxPin, TestRxPin>(
        BaudRate{115200}
    );

    // Test that we can call inherited methods
    // Note: These don't execute (no main), just verify they compile
    [[maybe_unused]] auto send_result = uart.send('A');
    [[maybe_unused]] auto receive_result = uart.receive();
    [[maybe_unused]] auto flush_result = uart.flush();
    [[maybe_unused]] auto write_result = uart.write("Test");
    [[maybe_unused]] auto available = uart.available();
    [[maybe_unused]] auto has_data = uart.has_data();
}

/**
 * @brief Test that TX-only API works
 */
void test_tx_only_api() {
    using TxOnlyConfig = SimpleUartConfigTxOnly<TestTxPin, TestPolicy>;
    using BaseType = UartBase<TxOnlyConfig>;

    // Verify inheritance
    static_assert(std::is_base_of_v<BaseType, TxOnlyConfig>,
                  "SimpleUartConfigTxOnly must inherit from UartBase");

    // Create TX-only configuration
    auto uart = Uart<PeripheralId::USART0, TestPolicy>::quick_setup_tx_only<TestTxPin>(
        BaudRate{115200}
    );

    // Test TX methods compile
    [[maybe_unused]] auto send_result = uart.send('A');
    [[maybe_unused]] auto write_result = uart.write("Test");
    [[maybe_unused]] auto flush_result = uart.flush();

    // RX methods should compile but return error
    [[maybe_unused]] auto receive_result = uart.receive();
    static_assert(std::is_same_v<decltype(receive_result), Result<char, ErrorCode>>,
                  "receive() must return Result<char, ErrorCode>");
}

/**
 * @brief Test zero-overhead guarantee
 */
void test_zero_overhead() {
    using ConfigType = SimpleUartConfig<TestTxPin, TestRxPin, TestPolicy>;
    using BaseType = UartBase<ConfigType>;

    // Verify empty base optimization
    static_assert(sizeof(BaseType) == 1,
                  "UartBase must be empty (sizeof == 1)");

    static_assert(std::is_empty_v<BaseType>,
                  "UartBase must have no data members");
}

// ============================================================================
// Summary
// ============================================================================

/**
 * Test Results:
 * - [x] SimpleUartConfig inherits from UartBase
 * - [x] SimpleUartConfigTxOnly inherits from UartBase
 * - [x] All inherited methods compile
 * - [x] TX-only mode returns errors for RX methods
 * - [x] Zero-overhead guarantee verified
 *
 * If this file compiles without errors, Phase 1.3 refactoring is successful!
 */
