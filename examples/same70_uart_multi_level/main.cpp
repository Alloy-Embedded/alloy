/**
 * @file main.cpp
 * @brief SAME70 UART Multi-Level API Example
 *
 * Demonstrates all 3 levels of UART configuration APIs:
 * - Level 1: Simple API (one-liner)
 * - Level 2: Fluent API (method chaining)
 * - Level 3: Expert API (configuration as data)
 *
 * Hardware: SAME70 Xplained Board
 * - USART0 TX: PD3
 * - USART0 RX: PD4
 * - USART1 TX: PA22
 * - USART1 RX: PA21
 */

#include "hal/platform/same70/gpio.hpp"
#include "hal/uart_simple.hpp"
#include "hal/uart_fluent.hpp"
#include "hal/uart_expert.hpp"

using namespace alloy::hal;
using namespace alloy::hal::same70;
using namespace alloy::hal::signals;
using namespace alloy::core;

// =============================================================================
// Pin Definitions
// =============================================================================

// USART0 pins
using Usart0_TX = GpioPin<PIOD_BASE, 3>;
using Usart0_RX = GpioPin<PIOD_BASE, 4>;

// USART1 pins
using Usart1_TX = GpioPin<PIOA_BASE, 22>;
using Usart1_RX = GpioPin<PIOA_BASE, 21>;

// =============================================================================
// Example 1: Simple API - For Beginners
// =============================================================================

void example_simple_api() {
    // One-liner UART setup with sensible defaults (8N1, 115200)
    auto uart = Uart<PeripheralId::USART0>::quick_setup<Usart0_TX, Usart0_RX>(
        BaudRate{115200});

    // Initialize and check for errors
    auto result = uart.initialize();
    if (result.is_ok()) {
        // UART is ready to use!
        // uart.write("Hello from Simple API!\n");
    }
}

void example_simple_api_tx_only() {
    // TX-only configuration for logging/debug
    auto logger = Uart<PeripheralId::USART1>::quick_setup_tx_only<Usart1_TX>(
        BaudRate{115200});

    auto result = logger.initialize();
    if (result.is_ok()) {
        // Logger ready for output
        // logger.write("Debug message\n");
    }
}

void example_simple_api_custom_parity() {
    // Custom parity for specific protocols
    auto uart = Uart<PeripheralId::USART0>::quick_setup<Usart0_TX, Usart0_RX>(
        BaudRate{9600}, UartParity::EVEN);

    auto result = uart.initialize();
    // Ready for 9600 8E1 communication
}

// =============================================================================
// Example 2: Fluent API - For Intermediate Users
// =============================================================================

void example_fluent_api_basic() {
    // Readable configuration with method chaining
    auto uart = UartBuilder<PeripheralId::USART0>()
        .with_tx_pin<Usart0_TX>()
        .with_rx_pin<Usart0_RX>()
        .baudrate(BaudRate{115200})
        .standard_8n1()
        .initialize();

    if (uart.is_ok()) {
        auto config = uart.unwrap();
        auto result = config.apply();
        // UART configured and ready
    }
}

void example_fluent_api_custom() {
    // More control over parameters
    auto uart = UartBuilder<PeripheralId::USART1>()
        .with_pins<Usart1_TX, Usart1_RX>()
        .baudrate(BaudRate{921600})
        .data_bits(8)
        .parity(UartParity::ODD)
        .stop_bits(1)
        .flow_control(false)
        .initialize();

    if (uart.is_ok()) {
        // High-speed UART with odd parity
    }
}

void example_fluent_api_preset() {
    // Using preset configurations
    auto uart = UartBuilder<PeripheralId::USART0>()
        .with_pins<Usart0_TX, Usart0_RX>()
        .baudrate(BaudRate{19200})
        .standard_8e1()  // 8 data bits, even parity, 1 stop bit
        .initialize();

    if (uart.is_ok()) {
        // 19200 8E1 UART ready
    }
}

// =============================================================================
// Example 3: Expert API - For Advanced Users
// =============================================================================

void example_expert_api_basic() {
    // Configuration as data with compile-time validation
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

    // Validate at compile-time
    static_assert(config.is_valid(), config.error_message());

    // Apply configuration
    auto result = expert::configure(config);
    if (result.is_ok()) {
        // UART configured with full control
    }
}

void example_expert_api_preset() {
    // Using expert presets
    constexpr auto config = UartExpertConfig::standard_115200(
        PeripheralId::USART0,
        PinId::PD3,
        PinId::PD4
    );

    static_assert(config.is_valid());

    auto result = expert::configure(config);
    // Standard 115200 8N1 configuration
}

void example_expert_api_logger() {
    // TX-only logger configuration
    constexpr auto config = UartExpertConfig::logger_config(
        PeripheralId::USART1,
        PinId::PA22,
        BaudRate{115200}
    );

    static_assert(config.is_valid());

    auto result = expert::configure(config);
    // Optimized for logging output
}

void example_expert_api_dma() {
    // High-performance DMA configuration
    constexpr auto config = UartExpertConfig::dma_config(
        PeripheralId::USART0,
        PinId::PD3,
        PinId::PD4,
        BaudRate{921600}
    );

    static_assert(config.is_valid());
    static_assert(config.enable_dma_tx);
    static_assert(config.enable_dma_rx);

    auto result = expert::configure(config);
    // DMA-enabled high-speed UART
}

// =============================================================================
// Example 4: Compile-Time Validation
// =============================================================================

void example_compile_time_validation() {
    // This configuration is INVALID (no TX or RX enabled)
    // Uncommenting this will cause a compile-time error!

    /*
    constexpr UartExpertConfig invalid_config = {
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

    // This will fail at compile-time with a clear error message:
    // "Must enable at least TX or RX"
    static_assert(invalid_config.is_valid(), invalid_config.error_message());
    */
}

// =============================================================================
// Example 5: API Comparison
// =============================================================================

void compare_all_apis() {
    // All three achieve the same result: 115200 8N1 UART
    // Choose based on your needs:

    // Level 1: Simplest - just works
    auto simple = Uart<PeripheralId::USART0>::quick_setup<Usart0_TX, Usart0_RX>(
        BaudRate{115200});

    // Level 2: Readable - self-documenting
    auto fluent = UartBuilder<PeripheralId::USART0>()
        .with_pins<Usart0_TX, Usart0_RX>()
        .baudrate(BaudRate{115200})
        .standard_8n1()
        .initialize();

    // Level 3: Complete control - expert mode
    constexpr auto expert_cfg = UartExpertConfig::standard_115200(
        PeripheralId::USART0, PinId::PD3, PinId::PD4);
    static_assert(expert_cfg.is_valid());
    auto expert_result = expert::configure(expert_cfg);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    // Simple API examples
    example_simple_api();
    example_simple_api_tx_only();
    example_simple_api_custom_parity();

    // Fluent API examples
    example_fluent_api_basic();
    example_fluent_api_custom();
    example_fluent_api_preset();

    // Expert API examples
    example_expert_api_basic();
    example_expert_api_preset();
    example_expert_api_logger();
    example_expert_api_dma();

    // Comparison
    compare_all_apis();

    // Infinite loop (embedded application)
    while (true) {
        // Main application logic would go here
    }

    return 0;
}
