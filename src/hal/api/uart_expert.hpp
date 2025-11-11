/**
 * @file uart_expert.hpp
 * @brief Level 3 Expert API for UART Configuration
 *
 * Provides an advanced configuration API with consteval validation,
 * detailed error messages, and full control over all parameters.
 *
 * Design Principles:
 * - Configuration as data (declarative style)
 * - Compile-time validation with consteval
 * - Custom error messages for each validation failure
 * - Support both compile-time and runtime configs
 * - Zero runtime overhead when used at compile-time
 * - Expert-level control over all hardware details
 *
 * Example Usage:
 * @code
 * // Compile-time configuration with validation
 * constexpr UartExpertConfig config = {
 *     .peripheral = PeripheralId::USART0,
 *     .tx_pin = PinId::PD3,
 *     .rx_pin = PinId::PD4,
 *     .baudrate = BaudRate{115200},
 *     .data_bits = 8,
 *     .parity = UartParity::NONE,
 *     .stop_bits = 1,
 *     .flow_control = false,
 *     .enable_tx = true,
 *     .enable_rx = true
 * };
 *
 * // Validate at compile-time
 * static_assert(config.is_valid(), config.error_message());
 *
 * // Apply configuration
 * auto result = Uart::configure(config);
 * @endcode
 *
 * @note Part of Phase 4.3: Expert API
 * @see openspec/changes/modernize-peripheral-architecture/specs/multi-level-api/spec.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "core/units.hpp"
#include "hal/signals.hpp"
#include "hal/api/uart_simple.hpp"  // For UartParity

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Expert Configuration Structure
// ============================================================================

/**
 * @brief Expert-level UART configuration
 *
 * Complete configuration structure with all UART parameters.
 * Can be validated at compile-time or runtime.
 *
 * @tparam HardwarePolicy Hardware policy for platform-specific operations
 */
template <typename HardwarePolicy>
struct UartExpertConfig {
    // Core configuration
    PeripheralId peripheral;
    PinId tx_pin;
    PinId rx_pin;
    BaudRate baudrate;
    u8 data_bits;
    UartParity parity;
    u8 stop_bits;
    bool flow_control;

    // Advanced options
    bool enable_tx;
    bool enable_rx;
    bool enable_interrupts;
    bool enable_dma_tx;
    bool enable_dma_rx;

    // Hardware-specific options
    bool enable_oversampling;  // 16x vs 8x oversampling
    bool enable_rx_timeout;
    u32 rx_timeout_value;  // in bit periods

    // ========================================================================
    // Constexpr Validation
    // ========================================================================

    /**
     * @brief Check if configuration is valid
     *
     * Performs comprehensive validation of all parameters.
     * Can be evaluated at compile-time.
     *
     * @return true if valid, false otherwise
     */
    constexpr bool is_valid() const {
        // Must enable at least TX or RX
        if (!enable_tx && !enable_rx) {
            return false;
        }

        // Data bits must be 7, 8, or 9
        if (data_bits < 7 || data_bits > 9) {
            return false;
        }

        // Stop bits must be 1 or 2
        if (stop_bits < 1 || stop_bits > 2) {
            return false;
        }

        // Baud rate must be reasonable (300 to 10MHz)
        if (baudrate.value() < 300 || baudrate.value() > 10000000) {
            return false;
        }

        // If RX timeout enabled, value must be non-zero
        if (enable_rx_timeout && rx_timeout_value == 0) {
            return false;
        }

        // Flow control requires both TX and RX
        if (flow_control && (!enable_tx || !enable_rx)) {
            return false;
        }

        return true;
    }

    /**
     * @brief Get detailed error message
     *
     * Provides a descriptive error message explaining what's wrong
     * with the configuration.
     *
     * @return Error message string, or "Valid" if no errors
     */
    constexpr const char* error_message() const {
        if (!enable_tx && !enable_rx) {
            return "Must enable at least TX or RX";
        }

        if (data_bits < 7 || data_bits > 9) {
            return "Data bits must be 7, 8, or 9";
        }

        if (stop_bits < 1 || stop_bits > 2) {
            return "Stop bits must be 1 or 2";
        }

        if (baudrate.value() < 300) {
            return "Baud rate too low (minimum 300)";
        }

        if (baudrate.value() > 10000000) {
            return "Baud rate too high (maximum 10MHz)";
        }

        if (enable_rx_timeout && rx_timeout_value == 0) {
            return "RX timeout enabled but value is zero";
        }

        if (flow_control && !enable_tx) {
            return "Flow control requires TX to be enabled";
        }

        if (flow_control && !enable_rx) {
            return "Flow control requires RX to be enabled";
        }

        return "Valid";
    }

    /**
     * @brief Get configuration hint
     *
     * Provides suggestions for fixing invalid configurations.
     *
     * @return Hint string
     */
    constexpr const char* hint() const {
        if (!is_valid()) {
            return "Check error_message() for details on what to fix";
        }
        return "Configuration is valid";
    }

    // ========================================================================
    // Preset Configurations
    // ========================================================================

    /**
     * @brief Create standard 115200 8N1 configuration
     *
     * Common configuration for most UART applications.
     */
    static constexpr UartExpertConfig<HardwarePolicy> standard_115200(
        PeripheralId peripheral,
        PinId tx_pin,
        PinId rx_pin) {
        return UartExpertConfig<HardwarePolicy>{
            .peripheral = peripheral,
            .tx_pin = tx_pin,
            .rx_pin = rx_pin,
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
    }

    /**
     * @brief Create TX-only logger configuration
     *
     * Optimized for logging/debug output.
     */
    static constexpr UartExpertConfig<HardwarePolicy> logger_config(
        PeripheralId peripheral,
        PinId tx_pin,
        BaudRate baudrate = BaudRate{115200}) {
        return UartExpertConfig<HardwarePolicy>{
            .peripheral = peripheral,
            .tx_pin = tx_pin,
            .rx_pin = PinId::PA0,  // Unused
            .baudrate = baudrate,
            .data_bits = 8,
            .parity = UartParity::NONE,
            .stop_bits = 1,
            .flow_control = false,
            .enable_tx = true,
            .enable_rx = false,  // TX only
            .enable_interrupts = false,
            .enable_dma_tx = false,
            .enable_dma_rx = false,
            .enable_oversampling = true,
            .enable_rx_timeout = false,
            .rx_timeout_value = 0
        };
    }

    /**
     * @brief Create DMA-enabled configuration
     *
     * High-performance configuration with DMA.
     */
    static constexpr UartExpertConfig<HardwarePolicy> dma_config(
        PeripheralId peripheral,
        PinId tx_pin,
        PinId rx_pin,
        BaudRate baudrate) {
        return UartExpertConfig<HardwarePolicy>{
            .peripheral = peripheral,
            .tx_pin = tx_pin,
            .rx_pin = rx_pin,
            .baudrate = baudrate,
            .data_bits = 8,
            .parity = UartParity::NONE,
            .stop_bits = 1,
            .flow_control = false,
            .enable_tx = true,
            .enable_rx = true,
            .enable_interrupts = true,  // DMA needs interrupts
            .enable_dma_tx = true,
            .enable_dma_rx = true,
            .enable_oversampling = true,
            .enable_rx_timeout = false,
            .rx_timeout_value = 0
        };
    }
};

// ============================================================================
// Expert Configuration API
// ============================================================================

/**
 * @brief Expert UART configuration namespace
 *
 * Provides functions for working with expert configurations.
 */
namespace expert {

/**
 * @brief Configure UART with expert configuration
 *
 * Applies a validated configuration to the UART peripheral using hardware policy.
 *
 * @tparam HardwarePolicy Hardware policy for platform-specific operations
 * @param config Expert configuration struct
 * @return Result indicating success or error
 *
 * @note Configuration must be valid (config.is_valid() == true)
 */
template <typename HardwarePolicy>
inline Result<void, ErrorCode> configure(const UartExpertConfig<HardwarePolicy>& config) {
    // Validate configuration
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    // TODO: Configure GPIO pins for TX/RX

    // Reset UART peripheral
    HardwarePolicy::reset();

    // Configure data format (data bits, parity, stop bits)
    if (config.parity == UartParity::NONE && config.data_bits == 8 && config.stop_bits == 1) {
        HardwarePolicy::configure_8n1();
    }
    // TODO: Handle other parity/data/stop bit combinations

    // Set baud rate
    HardwarePolicy::set_baudrate(config.baudrate.value());

    // Enable TX/RX as needed
    if (config.enable_tx) {
        HardwarePolicy::enable_tx();
    }
    if (config.enable_rx) {
        HardwarePolicy::enable_rx();
    }

    // TODO: Configure DMA if enabled
    // TODO: Enable interrupts if needed

    return Ok();
}

// NOTE: Non-type template parameters with UartExpertConfig are not supported
// because BaudRate has private members. Use constexpr validation instead:
//
// constexpr auto config = UartExpertConfig::standard_115200(...);
// static_assert(config.is_valid(), config.error_message());

}  // namespace expert

// ============================================================================
// Compile-Time Validation Helpers
// ============================================================================

/**
 * @brief Validate UART configuration at compile-time
 *
 * Use in static_assert to ensure configuration is valid.
 *
 * Example:
 * @code
 * constexpr auto config = UartExpertConfig<Policy>::standard_115200(...);
 * static_assert(validate_uart_config(config), "Invalid UART config");
 * @endcode
 */
template <typename HardwarePolicy>
constexpr bool validate_uart_config(const UartExpertConfig<HardwarePolicy>& config) {
    return config.is_valid();
}

/**
 * @brief Check specific validation rules
 */
template <typename HardwarePolicy>
constexpr bool has_valid_baudrate(const UartExpertConfig<HardwarePolicy>& config) {
    return config.baudrate.value() >= 300 && config.baudrate.value() <= 10000000;
}

template <typename HardwarePolicy>
constexpr bool has_valid_data_bits(const UartExpertConfig<HardwarePolicy>& config) {
    return config.data_bits >= 7 && config.data_bits <= 9;
}

template <typename HardwarePolicy>
constexpr bool has_valid_stop_bits(const UartExpertConfig<HardwarePolicy>& config) {
    return config.stop_bits >= 1 && config.stop_bits <= 2;
}

template <typename HardwarePolicy>
constexpr bool has_enabled_direction(const UartExpertConfig<HardwarePolicy>& config) {
    return config.enable_tx || config.enable_rx;
}

}  // namespace alloy::hal
