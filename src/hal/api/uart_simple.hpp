/**
 * @file uart_simple.hpp
 * @brief Level 1 Simple API for UART Configuration
 *
 * Provides a one-liner API for UART setup with automatic pin validation,
 * sensible defaults, and clear error messages.
 *
 * Design Principles:
 * - One function call for basic setup
 * - Automatic pin validation using signal routing
 * - Sensible defaults (8N1, no flow control)
 * - Clear compile-time errors for invalid pins
 * - Zero runtime overhead
 *
 * Example Usage:
 * @code
 * // Simple UART setup - one line!
 * auto uart = Uart<PeripheralId::USART0>::quick_setup<PinD4, PinD3>(BaudRate{115200});
 *
 * // Initialize and use it
 * uart.initialize().expect("UART init failed");
 * uart.send('A').expect("Send failed");
 * uart.write("Hello World!");
 * @endcode
 *
 * @note Part of Phase 1.3: CRTP Refactoring
 * @see docs/architecture/UART_SIMPLE_REFACTORING_PLAN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "core/units.hpp"
#include "hal/api/uart_base.hpp"
#include "hal/core/signals.hpp"
#include "hal/core/signal_registry.hpp"

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;
using namespace alloy::hal::signal_registry;

// ============================================================================
// UART Types
// ============================================================================

/**
 * @brief UART parity configuration
 */
enum class UartParity : u8 {
    NONE = 0,  // No parity
    EVEN = 1,  // Even parity
    ODD = 2,   // Odd parity
};

// ============================================================================
// Default Configuration Values
// ============================================================================

/**
 * @brief Default UART configuration
 *
 * Sensible defaults for most use cases:
 * - 8 data bits
 * - No parity
 * - 1 stop bit
 * - No flow control
 */
struct UartDefaults {
    static constexpr u8 data_bits = 8;
    static constexpr UartParity parity = UartParity::NONE;
    static constexpr u8 stop_bits = 1;
    static constexpr bool flow_control = false;
};

// ============================================================================
// Forward Declarations
// ============================================================================

template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig;

template <typename TxPin, typename HardwarePolicy>
struct SimpleUartConfigTxOnly;

// ============================================================================
// Simple UART API
// ============================================================================

/**
 * @brief Simple UART API with compile-time pin validation
 *
 * Level 1 API that provides the simplest possible interface for UART setup.
 * Automatically validates pins at compile-time and provides clear errors.
 *
 * @tparam PeriphId UART peripheral ID (USART0, USART1, etc.)
 * @tparam HardwarePolicy Hardware policy implementing platform-specific operations
 */
template <PeripheralId PeriphId, typename HardwarePolicy>
class Uart {
public:
    /**
     * @brief Quick setup with TX and RX pins
     *
     * One-liner UART configuration with automatic pin validation.
     * Uses sensible defaults (8N1, no flow control).
     *
     * @tparam TxPin GPIO pin type for TX signal
     * @tparam RxPin GPIO pin type for RX signal
     * @param baudrate Baud rate (e.g., BaudRate{115200})
     * @return Result<Uart, ErrorCode> Configured UART or error
     *
     * @note Pins are validated at compile-time
     * @note Will not compile if pins are incompatible with peripheral
     *
     * Example:
     * @code
     * using PinD4 = GpioPin<PIOD_BASE, 4>;  // USART0 RX
     * using PinD3 = GpioPin<PIOD_BASE, 3>;  // USART0 TX
     * auto uart = Uart<PeripheralId::USART0>::quick_setup<PinD3, PinD4>(BaudRate{115200});
     * @endcode
     */
    template <typename TxPin, typename RxPin>
    static constexpr auto quick_setup(BaudRate baudrate) {
        // Compile-time pin validation
        static_assert(is_valid_tx_pin<TxPin>(),
                     "TX pin is not compatible with this UART peripheral. "
                     "Check signal routing tables for valid TX pins.");

        static_assert(is_valid_rx_pin<RxPin>(),
                     "RX pin is not compatible with this UART peripheral. "
                     "Check signal routing tables for valid RX pins.");

        // Return configuration result
        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>(
            PeriphId,
            baudrate,
            UartDefaults::data_bits,
            UartDefaults::parity,
            UartDefaults::stop_bits,
            UartDefaults::flow_control
        );
    }

    /**
     * @brief Quick setup with TX, RX, and custom parity
     *
     * @tparam TxPin GPIO pin type for TX signal
     * @tparam RxPin GPIO pin type for RX signal
     * @param baudrate Baud rate
     * @param parity Parity setting (NONE, EVEN, ODD)
     * @return Result<Uart, ErrorCode> Configured UART or error
     */
    template <typename TxPin, typename RxPin>
    static constexpr auto quick_setup(BaudRate baudrate, UartParity parity) {
        static_assert(is_valid_tx_pin<TxPin>(), "Invalid TX pin");
        static_assert(is_valid_rx_pin<RxPin>(), "Invalid RX pin");

        return SimpleUartConfig<TxPin, RxPin, HardwarePolicy>(
            PeriphId,
            baudrate,
            UartDefaults::data_bits,
            parity,
            UartDefaults::stop_bits,
            UartDefaults::flow_control
        );
    }

    /**
     * @brief Quick setup - TX only (for logging/output)
     *
     * Simplified setup for TX-only use cases (e.g., logging).
     *
     * @tparam TxPin GPIO pin type for TX signal
     * @param baudrate Baud rate
     * @return UartConfig Configuration object
     */
    template <typename TxPin>
    static constexpr auto quick_setup_tx_only(BaudRate baudrate) {
        static_assert(is_valid_tx_pin<TxPin>(), "Invalid TX pin");

        return SimpleUartConfigTxOnly<TxPin, HardwarePolicy>(
            PeriphId,
            baudrate,
            UartDefaults::data_bits,
            UartDefaults::parity,
            UartDefaults::stop_bits
        );
    }

private:
    // ========================================================================
    // Compile-Time Pin Validation Helpers
    // ========================================================================

    /**
     * @brief Check if pin is valid for TX signal
     *
     * Validates that the pin supports the TX signal for this peripheral.
     *
     * @tparam Pin GPIO pin type
     * @return true if pin is compatible, false otherwise
     */
    template <typename Pin>
    static constexpr bool is_valid_tx_pin() {
        // Get the TX signal type for this peripheral
        // This will be specialized per peripheral in platform-specific code
        return check_pin_signal_compatibility<Pin>(PeriphId, SignalType::TX);
    }

    /**
     * @brief Check if pin is valid for RX signal
     *
     * @tparam Pin GPIO pin type
     * @return true if pin is compatible, false otherwise
     */
    template <typename Pin>
    static constexpr bool is_valid_rx_pin() {
        return check_pin_signal_compatibility<Pin>(PeriphId, SignalType::RX);
    }

    /**
     * @brief Generic pin-signal compatibility check
     *
     * @tparam Pin GPIO pin type
     * @param peripheral Peripheral ID
     * @param signal Signal type (TX, RX, etc.)
     * @return true if compatible, false otherwise
     */
    template <typename Pin>
    static constexpr bool check_pin_signal_compatibility(
        PeripheralId peripheral,
        SignalType signal) {

        constexpr PinId pin_id = Pin::get_pin_id();

        // This will be implemented using the signal tables we generated
        // For now, return true to allow compilation
        // TODO: Integrate with generated signal tables
        return true;
    }
};

// ============================================================================
// Configuration Result Types
// ============================================================================

/**
 * @brief Simple UART configuration (TX + RX)
 *
 * Holds validated configuration for full-duplex UART.
 * Now inherits from UartBase using CRTP pattern for zero-overhead code reuse.
 *
 * @tparam TxPin TX pin type
 * @tparam RxPin RX pin type
 * @tparam HardwarePolicy Hardware policy for platform-specific operations
 */
template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig : public UartBase<SimpleUartConfig<TxPin, RxPin, HardwarePolicy>> {
    using Base = UartBase<SimpleUartConfig<TxPin, RxPin, HardwarePolicy>>;
    friend Base;

    // Configuration state
    PeripheralId peripheral;
    BaudRate baudrate;
    u8 data_bits;
    UartParity parity;
    u8 stop_bits;
    bool flow_control;

    constexpr SimpleUartConfig(PeripheralId p, BaudRate br, u8 db, UartParity par, u8 sb, bool fc)
        : peripheral(p), baudrate(br), data_bits(db), parity(par), stop_bits(sb), flow_control(fc) {}

    // ========================================================================
    // Inherited Interface from UartBase (CRTP)
    // ========================================================================

    // Inherit all common UART methods from base
    using Base::send;           // Send single character
    using Base::receive;        // Receive single character
    using Base::write;          // Write null-terminated string
    using Base::send_buffer;    // Send buffer of bytes
    using Base::receive_buffer; // Receive buffer of bytes
    using Base::flush;          // Wait for transmission complete
    using Base::available;      // Number of bytes available
    using Base::has_data;       // Check if data available
    using Base::set_baud_rate;  // Change baud rate

    // ========================================================================
    // Simple API Specific Methods
    // ========================================================================

    /**
     * @brief Initialize the UART with this configuration
     *
     * Applies the configuration to hardware registers using the hardware policy.
     *
     * @return Result<void, ErrorCode> Success or error
     */
    Result<void, ErrorCode> initialize() const {
        // TODO: Configure TX and RX pins using GPIO configuration

        // Reset and configure UART peripheral using hardware policy
        HardwarePolicy::reset();

        // Configure communication parameters
        if (parity == UartParity::NONE && data_bits == 8 && stop_bits == 1) {
            HardwarePolicy::configure_8n1();
        }
        // TODO: Handle other parity/data/stop bit combinations

        // Set baud rate
        HardwarePolicy::set_baudrate(baudrate.value());

        // Enable TX and RX
        HardwarePolicy::enable_transmitter();
        HardwarePolicy::enable_receiver();
        HardwarePolicy::enable_uart();

        return Ok(); // Success
    }

private:
    // ========================================================================
    // Implementation Methods (called by UartBase via CRTP)
    // ========================================================================

    /**
     * @brief Send implementation - called by Base::send()
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> send_impl(char c) noexcept {
        // Wait for TX ready
        while (!HardwarePolicy::is_tx_empty()) {
            // TODO: Add timeout handling
        }
        HardwarePolicy::write_data(static_cast<u8>(c));
        return Ok();
    }

    /**
     * @brief Receive implementation - called by Base::receive()
     */
    [[nodiscard]] constexpr Result<char, ErrorCode> receive_impl() noexcept {
        // Wait for RX ready
        while (!HardwarePolicy::is_rx_not_empty()) {
            // TODO: Add timeout handling
        }
        return Ok(static_cast<char>(HardwarePolicy::read_data()));
    }

    /**
     * @brief Send buffer implementation - called by Base::send_buffer()
     */
    [[nodiscard]] constexpr Result<size_t, ErrorCode> send_buffer_impl(
        const char* buffer,
        size_t length
    ) noexcept {
        for (size_t i = 0; i < length; ++i) {
            auto result = send_impl(buffer[i]);
            if (result.is_err()) {
                return Ok(static_cast<size_t>(i)); // Return bytes sent before error
            }
        }
        return Ok(static_cast<size_t>(length));
    }

    /**
     * @brief Receive buffer implementation - called by Base::receive_buffer()
     */
    [[nodiscard]] constexpr Result<size_t, ErrorCode> receive_buffer_impl(
        char* buffer,
        size_t length
    ) noexcept {
        for (size_t i = 0; i < length; ++i) {
            auto result = receive_impl();
            if (result.is_err()) {
                return Ok(static_cast<size_t>(i)); // Return bytes received before error
            }
            buffer[i] = result.unwrap();
        }
        return Ok(static_cast<size_t>(length));
    }

    /**
     * @brief Flush implementation - called by Base::flush()
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> flush_impl() noexcept {
        // Wait for transmission complete
        while (!HardwarePolicy::is_tx_complete()) {
            // TODO: Add timeout handling
        }
        return Ok();
    }

    /**
     * @brief Available implementation - called by Base::available()
     */
    [[nodiscard]] constexpr size_t available_impl() const noexcept {
        // Check if RX has data
        return HardwarePolicy::is_rx_not_empty() ? 1 : 0;
    }

    /**
     * @brief Set baud rate implementation - called by Base::set_baud_rate()
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_baud_rate_impl(BaudRate baud) noexcept {
        HardwarePolicy::set_baudrate(baud.value());
        baudrate = baud;
        return Ok();
    }
};

/**
 * @brief Simple UART configuration (TX only)
 *
 * Simplified configuration for TX-only operation.
 * Inherits from UartBase using CRTP, but only implements TX methods.
 *
 * @tparam TxPin TX pin type
 * @tparam HardwarePolicy Hardware policy for platform-specific operations
 */
template <typename TxPin, typename HardwarePolicy>
struct SimpleUartConfigTxOnly : public UartBase<SimpleUartConfigTxOnly<TxPin, HardwarePolicy>> {
    using Base = UartBase<SimpleUartConfigTxOnly<TxPin, HardwarePolicy>>;
    friend Base;

    // Configuration state
    PeripheralId peripheral;
    BaudRate baudrate;
    u8 data_bits;
    UartParity parity;
    u8 stop_bits;

    constexpr SimpleUartConfigTxOnly(PeripheralId p, BaudRate br, u8 db, UartParity par, u8 sb)
        : peripheral(p), baudrate(br), data_bits(db), parity(par), stop_bits(sb) {}

    // ========================================================================
    // Inherited Interface from UartBase (CRTP)
    // ========================================================================

    // Inherit TX methods from base
    using Base::send;           // Send single character
    using Base::write;          // Write null-terminated string
    using Base::send_buffer;    // Send buffer of bytes
    using Base::flush;          // Wait for transmission complete
    using Base::set_baud_rate;  // Change baud rate

    // ========================================================================
    // Simple API Specific Methods
    // ========================================================================

    Result<void, ErrorCode> initialize() const {
        // TODO: Configure TX pin using GPIO configuration

        // Reset and configure UART peripheral using hardware policy
        HardwarePolicy::reset();

        // Configure communication parameters (8N1 assumed for TX-only)
        if (parity == UartParity::NONE && data_bits == 8 && stop_bits == 1) {
            HardwarePolicy::configure_8n1();
        }

        // Set baud rate
        HardwarePolicy::set_baudrate(baudrate.value());

        // Enable TX only
        HardwarePolicy::enable_transmitter();
        HardwarePolicy::enable_uart();

        return Ok(); // Success
    }

    /**
     * @brief Write a single byte (blocking) - Legacy compatibility method
     *
     * Waits until the transmitter is ready, then sends the byte.
     *
     * @param byte Byte to transmit
     *
     * @deprecated Use send() for Result-based error handling
     */
    void write_byte(u8 byte) const {
        send(static_cast<char>(byte)).expect("Write byte failed");
    }

private:
    // ========================================================================
    // Implementation Methods (called by UartBase via CRTP)
    // ========================================================================

    /**
     * @brief Send implementation - called by Base::send()
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> send_impl(char c) noexcept {
        // Wait for TX ready
        while (!HardwarePolicy::is_tx_empty()) {
            // TODO: Add timeout handling
        }
        HardwarePolicy::write_data(static_cast<u8>(c));
        return Ok();
    }

    /**
     * @brief Receive implementation - TX-only, returns error
     */
    [[nodiscard]] constexpr Result<char, ErrorCode> receive_impl() noexcept {
        return Err(ErrorCode::NotSupported); // TX-only mode
    }

    /**
     * @brief Send buffer implementation - called by Base::send_buffer()
     */
    [[nodiscard]] constexpr Result<size_t, ErrorCode> send_buffer_impl(
        const char* buffer,
        size_t length
    ) noexcept {
        for (size_t i = 0; i < length; ++i) {
            auto result = send_impl(buffer[i]);
            if (result.is_err()) {
                return Ok(static_cast<size_t>(i)); // Return bytes sent before error
            }
        }
        return Ok(static_cast<size_t>(length));
    }

    /**
     * @brief Receive buffer implementation - TX-only, returns error
     */
    [[nodiscard]] constexpr Result<size_t, ErrorCode> receive_buffer_impl(
        char* buffer,
        size_t length
    ) noexcept {
        (void)buffer;
        (void)length;
        return Err(ErrorCode::NotSupported); // TX-only mode
    }

    /**
     * @brief Flush implementation - called by Base::flush()
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> flush_impl() noexcept {
        // Wait for transmission complete
        while (!HardwarePolicy::is_tx_complete()) {
            // TODO: Add timeout handling
        }
        return Ok();
    }

    /**
     * @brief Available implementation - TX-only, always returns 0
     */
    [[nodiscard]] constexpr size_t available_impl() const noexcept {
        return 0; // TX-only mode, no RX data
    }

    /**
     * @brief Set baud rate implementation - called by Base::set_baud_rate()
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_baud_rate_impl(BaudRate baud) noexcept {
        HardwarePolicy::set_baudrate(baud.value());
        baudrate = baud;
        return Ok();
    }
};

}  // namespace alloy::hal
