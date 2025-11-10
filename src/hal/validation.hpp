/**
 * @file validation.hpp
 * @brief Compile-Time Validation Helpers with Custom Error Messages
 *
 * Provides consteval functions for validating peripheral configurations at compile-time
 * with clear, actionable error messages. Uses C++20 consteval to ensure all validation
 * happens during compilation, resulting in zero runtime overhead.
 *
 * Design Principles:
 * - All validation at compile-time using consteval
 * - Custom error messages that suggest fixes
 * - Clear indication of what's wrong and how to fix it
 * - Zero runtime overhead (no runtime checks)
 * - Composable validation functions
 *
 * @note Part of Phase 1: Validation Infrastructure
 * @see openspec/changes/modernize-peripheral-architecture/specs/concept-layer/spec.md
 */

#pragma once

#include <array>
#include <concepts>
#include <string_view>

#include "core/types.hpp"
#include "hal/signals.hpp"

namespace alloy::hal::validation {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Validation Result Types
// ============================================================================

/**
 * @brief Result of a compile-time validation
 *
 * Contains validation status and error message if validation failed.
 * Used by consteval functions to provide detailed error information.
 */
struct ValidationResult {
    bool is_valid;
    std::string_view error_message;
    std::string_view suggestion;

    constexpr ValidationResult(bool valid, std::string_view error = "",
                               std::string_view hint = "")
        : is_valid(valid), error_message(error), suggestion(hint) {}

    constexpr operator bool() const { return is_valid; }
};

/**
 * @brief Helper to create a successful validation result
 */
consteval ValidationResult Ok() {
    return ValidationResult{true, "", ""};
}

/**
 * @brief Helper to create a failed validation result with error message
 */
consteval ValidationResult Err(std::string_view error, std::string_view suggestion = "") {
    return ValidationResult{false, error, suggestion};
}

// ============================================================================
// Pin Signal Validation
// ============================================================================

/**
 * @brief Validate that a pin supports a specific peripheral signal
 *
 * Checks if the given pin can be used for the specified peripheral signal.
 * Provides detailed error message if validation fails.
 *
 * @tparam Signal The peripheral signal type (e.g., Usart1TxSignal)
 * @param pin The pin ID to validate
 * @return ValidationResult with status and error details
 *
 * Example usage:
 * @code
 * constexpr auto result = validate_pin_signal<Usart1TxSignal>(PinId::PA9);
 * static_assert(result.is_valid, result.error_message);
 * @endcode
 */
template <typename Signal>
consteval ValidationResult validate_pin_signal(PinId pin) {
    if constexpr (requires { Signal::compatible_pins; }) {
        // Check if pin is in compatible pins list
        for (const auto& pin_def : Signal::compatible_pins) {
            if (pin_def.pin == pin) {
                return Ok();
            }
        }

        // Pin not found - generate helpful error message
        // Note: In a real implementation, we'd list compatible pins here
        return Err(
            "Pin does not support this peripheral signal",
            "Check datasheet for compatible pins or use signal routing tables"
        );
    }

    return Err(
        "Signal type does not have pin compatibility information",
        "This signal may need to be generated from SVD first"
    );
}

/**
 * @brief Validate multiple pin/signal pairs at once
 *
 * Validates an array of pin/signal connections and returns the first error found.
 * Useful for validating entire peripheral configurations.
 *
 * @param pins Array of pin IDs
 * @param signals Array of signal types (must be same size as pins)
 * @return ValidationResult with status and error details
 */
template <usize N>
consteval ValidationResult validate_pin_signals(const std::array<PinId, N>& pins) {
    // Placeholder for future implementation when we have multiple signals
    // This will iterate through pins and validate each one
    return Ok();
}

// ============================================================================
// Configuration Field Validation
// ============================================================================

/**
 * @brief Validate that a numeric value is within a valid range
 *
 * Generic range validator with custom error messages.
 *
 * @tparam T The numeric type
 * @param value The value to validate
 * @param min Minimum valid value (inclusive)
 * @param max Maximum valid value (inclusive)
 * @param field_name Name of the field for error messages
 * @return ValidationResult with status and error details
 *
 * Example usage:
 * @code
 * constexpr auto result = validate_range(baudrate, 1200, 115200, "baudrate");
 * static_assert(result.is_valid, result.error_message);
 * @endcode
 */
template <typename T>
    requires std::integral<T> || std::floating_point<T>
consteval ValidationResult validate_range(T value, T min, T max, std::string_view field_name) {
    if (value < min || value > max) {
        return Err(
            "Value out of valid range",
            "Check peripheral datasheet for valid range"
        );
    }
    return Ok();
}

/**
 * @brief Validate that a value is one of a set of allowed values
 *
 * Validates that a value is in a compile-time list of valid options.
 *
 * @tparam T The value type
 * @tparam N Number of allowed values
 * @param value The value to validate
 * @param allowed Array of allowed values
 * @param field_name Name of the field for error messages
 * @return ValidationResult with status and error details
 */
template <typename T, usize N>
consteval ValidationResult validate_allowed(T value, const std::array<T, N>& allowed,
                                            std::string_view field_name) {
    for (const auto& allowed_value : allowed) {
        if (value == allowed_value) {
            return Ok();
        }
    }

    return Err(
        "Value not in allowed set",
        "Use one of the predefined constants"
    );
}

/**
 * @brief Validate that a required field is not zero/null
 *
 * Simple validator to ensure required configuration fields are provided.
 *
 * @tparam T The value type
 * @param value The value to validate
 * @param field_name Name of the field for error messages
 * @return ValidationResult with status and error details
 */
template <typename T>
consteval ValidationResult validate_required(T value, std::string_view field_name) {
    if (value == T{}) {
        return Err(
            "Required field not set",
            "This field must be explicitly configured"
        );
    }
    return Ok();
}

// ============================================================================
// DMA Configuration Validation
// ============================================================================

/**
 * @brief Validate DMA channel compatibility with peripheral
 *
 * Checks if a DMA channel can be used with a specific peripheral.
 *
 * @param dma_peripheral DMA controller peripheral ID
 * @param dma_channel DMA channel/stream number
 * @param peripheral Peripheral that will use DMA
 * @param request_type Type of DMA request (TX, RX, DATA)
 * @return ValidationResult with status and error details
 */
consteval ValidationResult validate_dma_compatibility(
    PeripheralId dma_peripheral,
    u8 dma_channel,
    PeripheralId peripheral,
    DmaRequestType request_type) {

    // Placeholder for future implementation when DMA compatibility tables are generated
    // This will check if the DMA channel supports the peripheral's request type

    return Ok();  // Assume valid for now
}

/**
 * @brief Check if a DMA channel is already allocated
 *
 * Prevents double-allocation of DMA channels at compile-time.
 * This will be implemented with a compile-time registry in the future.
 *
 * @param dma_peripheral DMA controller peripheral ID
 * @param dma_channel DMA channel/stream number
 * @return ValidationResult with status and error details
 */
consteval ValidationResult validate_dma_not_allocated(
    PeripheralId dma_peripheral,
    u8 dma_channel) {

    // Placeholder for future implementation with DMA registry
    // This will check a compile-time list of allocated DMA channels

    return Ok();  // Assume not allocated for now
}

// ============================================================================
// Peripheral Configuration Validation
// ============================================================================

/**
 * @brief Validate UART configuration
 *
 * Checks all UART configuration parameters for validity.
 *
 * @param baudrate Baud rate in bits per second
 * @param data_bits Number of data bits (typically 7, 8, or 9)
 * @param parity Parity setting (0=None, 1=Even, 2=Odd)
 * @param stop_bits Number of stop bits (1 or 2)
 * @return ValidationResult with status and error details
 */
consteval ValidationResult validate_uart_config(
    u32 baudrate,
    u8 data_bits,
    u8 parity,
    u8 stop_bits) {

    // Validate baudrate (common range: 1200 to 4000000)
    if (baudrate < 1200 || baudrate > 4000000) {
        return Err(
            "Invalid baud rate",
            "Use standard baud rates: 9600, 19200, 38400, 57600, 115200, etc."
        );
    }

    // Validate data bits (7, 8, or 9)
    if (data_bits < 7 || data_bits > 9) {
        return Err(
            "Invalid data bits",
            "Use 7, 8, or 9 data bits"
        );
    }

    // Validate parity (0=None, 1=Even, 2=Odd)
    if (parity > 2) {
        return Err(
            "Invalid parity setting",
            "Use Parity::None, Parity::Even, or Parity::Odd"
        );
    }

    // Validate stop bits (1 or 2)
    if (stop_bits < 1 || stop_bits > 2) {
        return Err(
            "Invalid stop bits",
            "Use StopBits::One or StopBits::Two"
        );
    }

    return Ok();
}

/**
 * @brief Validate SPI configuration
 *
 * Checks all SPI configuration parameters for validity.
 *
 * @param clock_speed SPI clock speed in Hz
 * @param mode SPI mode (0-3)
 * @param data_size Data frame size in bits (8 or 16)
 * @return ValidationResult with status and error details
 */
consteval ValidationResult validate_spi_config(
    u32 clock_speed,
    u8 mode,
    u8 data_size) {

    // Validate clock speed (typically up to 50 MHz for most MCUs)
    if (clock_speed == 0 || clock_speed > 50000000) {
        return Err(
            "Invalid SPI clock speed",
            "Use a value between 1 Hz and 50 MHz"
        );
    }

    // Validate mode (0-3)
    if (mode > 3) {
        return Err(
            "Invalid SPI mode",
            "Use SpiMode::Mode0, Mode1, Mode2, or Mode3"
        );
    }

    // Validate data size (8 or 16)
    if (data_size != 8 && data_size != 16) {
        return Err(
            "Invalid SPI data size",
            "Use SpiDataSize::Bits8 or SpiDataSize::Bits16"
        );
    }

    return Ok();
}

/**
 * @brief Validate timer configuration
 *
 * Checks timer configuration parameters for validity.
 *
 * @param period Timer period in ticks
 * @param prescaler Clock prescaler value
 * @return ValidationResult with status and error details
 */
consteval ValidationResult validate_timer_config(u32 period, u16 prescaler) {
    // Validate period (must be > 0 and < max 32-bit value)
    if (period == 0) {
        return Err(
            "Invalid timer period",
            "Period must be greater than 0"
        );
    }

    // Validate prescaler (typically 16-bit)
    if (prescaler == 0) {
        return Err(
            "Invalid prescaler",
            "Prescaler must be greater than 0"
        );
    }

    return Ok();
}

// ============================================================================
// Composite Validation
// ============================================================================

/**
 * @brief Combine multiple validation results
 *
 * Runs multiple validators and returns the first error found.
 * Useful for validating complex configurations with many fields.
 *
 * @tparam Validators Variadic template of validation functions
 * @param validators Functions that return ValidationResult
 * @return ValidationResult with status and first error found
 *
 * Example usage:
 * @code
 * constexpr auto result = validate_all(
 *     validate_range(baudrate, 1200, 115200, "baudrate"),
 *     validate_required(data_bits, "data_bits"),
 *     validate_pin_signal<Usart1TxSignal>(tx_pin)
 * );
 * static_assert(result.is_valid, result.error_message);
 * @endcode
 */
template <typename... Validators>
consteval ValidationResult validate_all(Validators... validators) {
    ValidationResult results[] = {validators...};

    for (const auto& result : results) {
        if (!result.is_valid) {
            return result;  // Return first error
        }
    }

    return Ok();
}

// ============================================================================
// Error Message Formatting
// ============================================================================

/**
 * @brief Format a validation error into a user-friendly message
 *
 * Creates a formatted error string with field name, error, and suggestion.
 *
 * @param field_name Name of the configuration field
 * @param result Validation result
 * @return Formatted error message
 */
consteval std::string_view format_error(std::string_view field_name,
                                        const ValidationResult& result) {
    if (result.is_valid) {
        return "";
    }

    // In a full implementation, we'd concatenate strings here
    // For now, just return the error message
    return result.error_message;
}

}  // namespace alloy::hal::validation
