/**
 * @file uart_fluent.hpp
 * @brief Level 2 Fluent API for UART Configuration
 *
 * Provides a builder pattern with method chaining for flexible UART setup.
 * Offers more control than Simple API while maintaining readability.
 *
 * Design Principles:
 * - Fluent method chaining (returns *this)
 * - Incremental validation across method calls
 * - Self-documenting API (reads like natural language)
 * - Compile-time validation where possible
 * - Clear error messages at initialization time
 *
 * Example Usage:
 * @code
 * auto config = UartBuilder<PeripheralId::USART0, HardwarePolicy>()
 *     .with_tx_pin<PinD3>()
 *     .with_rx_pin<PinD4>()
 *     .baudrate(BaudRate{115200})
 *     .parity(UartParity::EVEN)
 *     .data_bits(8)
 *     .stop_bits(1)
 *     .initialize()
 *     .expect("UART init failed");
 *
 * config.apply().expect("UART apply failed");
 * config.send('A').expect("Send failed");  // Now has send/receive from UartBase!
 * @endcode
 *
 * @note Part of Phase 1.4: CRTP Refactoring
 * @see docs/architecture/UART_CRTP_INTEGRATION.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "core/units.hpp"
#include "hal/api/uart_base.hpp"
#include "hal/core/signals.hpp"
#include "hal/core/signal_registry.hpp"
#include "hal/api/uart_simple.hpp"  // For UartParity and defaults

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Fluent Configuration Result
// ============================================================================

/**
 * @brief UART configuration from fluent builder
 *
 * Holds the complete validated configuration from the builder.
 * Now inherits from UartBase using CRTP for zero-overhead code reuse.
 *
 * @tparam HardwarePolicy Hardware policy for platform-specific operations
 */
template <typename HardwarePolicy>
struct FluentUartConfig : public UartBase<FluentUartConfig<HardwarePolicy>> {
    using Base = UartBase<FluentUartConfig<HardwarePolicy>>;
    friend Base;

    // Configuration state
    PeripheralId peripheral;
    PinId tx_pin;
    PinId rx_pin;
    BaudRate baudrate;
    UartParity parity;
    u8 data_bits;
    u8 stop_bits;
    bool flow_control;
    bool has_tx;
    bool has_rx;

    // Constructor to allow initialization from UartBuilder
    constexpr FluentUartConfig(
        PeripheralId p,
        PinId tx,
        PinId rx,
        BaudRate baud,
        UartParity par,
        u8 db,
        u8 sb,
        bool fc,
        bool tx_enabled,
        bool rx_enabled
    ) : peripheral(p), tx_pin(tx), rx_pin(rx), baudrate(baud),
        parity(par), data_bits(db), stop_bits(sb), flow_control(fc),
        has_tx(tx_enabled), has_rx(rx_enabled) {}

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
    // Fluent API Specific Methods
    // ========================================================================

    /**
     * @brief Apply configuration to hardware
     *
     * Configures GPIO pins and UART peripheral registers using hardware policy.
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> apply() const {
        // TODO: Configure TX pin if has_tx
        // TODO: Configure RX pin if has_rx

        // Reset and configure UART peripheral using hardware policy
        HardwarePolicy::reset();

        // Configure communication parameters
        if (parity == UartParity::NONE && data_bits == 8 && stop_bits == 1) {
            HardwarePolicy::configure_8n1();
        }
        // TODO: Handle other parity/data/stop bit combinations

        // Set baud rate
        HardwarePolicy::set_baudrate(baudrate.value());

        // Enable TX and/or RX based on configuration
        if (has_tx) {
            HardwarePolicy::enable_transmitter();
        }
        if (has_rx) {
            HardwarePolicy::enable_receiver();
        }
        HardwarePolicy::enable_uart();

        return Ok();
    }

private:
    // ========================================================================
    // Implementation Methods (called by UartBase via CRTP)
    // ========================================================================

    /**
     * @brief Send implementation - called by Base::send()
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> send_impl(char c) noexcept {
        if (!has_tx) {
            return Err(ErrorCode::NotSupported);
        }

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
        if (!has_rx) {
            return Err(ErrorCode::NotSupported);
        }

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
        if (!has_tx) {
            return Err(ErrorCode::NotSupported);
        }

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
        if (!has_rx) {
            return Err(ErrorCode::NotSupported);
        }

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
        if (!has_tx) {
            return Err(ErrorCode::NotSupported);
        }

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
        if (!has_rx) {
            return 0;
        }
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

// ============================================================================
// Builder State Tracking (Compile-Time)
// ============================================================================

/**
 * @brief Builder state flags
 *
 * Tracks which configuration options have been set.
 * Used for compile-time and runtime validation.
 */
struct BuilderState {
    bool has_tx_pin = false;
    bool has_rx_pin = false;
    bool has_baudrate = false;
    bool has_parity = false;
    bool has_data_bits = false;
    bool has_stop_bits = false;

    constexpr bool is_tx_only_valid() const {
        return has_tx_pin && !has_rx_pin && has_baudrate;
    }

    constexpr bool is_rx_only_valid() const {
        return !has_tx_pin && has_rx_pin && has_baudrate;
    }

    constexpr bool is_full_duplex_valid() const {
        return has_tx_pin && has_rx_pin && has_baudrate;
    }

    constexpr bool is_valid() const {
        return is_tx_only_valid() || is_rx_only_valid() || is_full_duplex_valid();
    }
};

// ============================================================================
// Fluent UART Builder
// ============================================================================

/**
 * @brief Fluent API builder for UART configuration
 *
 * Level 2 API that provides a readable, self-documenting interface
 * with method chaining.
 *
 * @tparam PeriphId UART peripheral ID
 * @tparam HardwarePolicy Hardware policy implementing platform-specific operations
 */
template <PeripheralId PeriphId, typename HardwarePolicy>
class UartBuilder {
public:
    /**
     * @brief Construct a new UART builder
     *
     * Starts with default values from UartDefaults.
     */
    constexpr UartBuilder()
        : baudrate_(BaudRate{9600}),  // Safe default
          parity_(UartDefaults::parity),
          data_bits_(UartDefaults::data_bits),
          stop_bits_(UartDefaults::stop_bits),
          flow_control_(UartDefaults::flow_control),
          tx_pin_id_(PinId::PA0),  // Placeholder
          rx_pin_id_(PinId::PA0),  // Placeholder
          state_{} {}

    // ========================================================================
    // Pin Configuration Methods
    // ========================================================================

    /**
     * @brief Set TX pin
     *
     * Configures the transmit pin for this UART.
     *
     * @tparam TxPin GPIO pin type
     * @return Reference to this builder for chaining
     *
     * Example:
     * @code
     * builder.with_tx_pin<PinD3>()
     * @endcode
     */
    template <typename TxPin>
    constexpr UartBuilder& with_tx_pin() {
        tx_pin_id_ = TxPin::get_pin_id();
        state_.has_tx_pin = true;
        return *this;
    }

    /**
     * @brief Set RX pin
     *
     * Configures the receive pin for this UART.
     *
     * @tparam RxPin GPIO pin type
     * @return Reference to this builder for chaining
     */
    template <typename RxPin>
    constexpr UartBuilder& with_rx_pin() {
        rx_pin_id_ = RxPin::get_pin_id();
        state_.has_rx_pin = true;
        return *this;
    }

    /**
     * @brief Set both TX and RX pins
     *
     * Convenience method to set both pins at once.
     *
     * @tparam TxPin GPIO pin type for TX
     * @tparam RxPin GPIO pin type for RX
     * @return Reference to this builder for chaining
     */
    template <typename TxPin, typename RxPin>
    constexpr UartBuilder& with_pins() {
        with_tx_pin<TxPin>();
        with_rx_pin<RxPin>();
        return *this;
    }

    // ========================================================================
    // Communication Parameter Methods
    // ========================================================================

    /**
     * @brief Set baud rate
     *
     * @param baud Baud rate value
     * @return Reference to this builder for chaining
     *
     * Example:
     * @code
     * builder.baudrate(BaudRate{115200})
     * @endcode
     */
    constexpr UartBuilder& baudrate(BaudRate baud) {
        baudrate_ = baud;
        state_.has_baudrate = true;
        return *this;
    }

    /**
     * @brief Set parity
     *
     * @param p Parity setting (NONE, EVEN, ODD)
     * @return Reference to this builder for chaining
     */
    constexpr UartBuilder& parity(UartParity p) {
        parity_ = p;
        state_.has_parity = true;
        return *this;
    }

    /**
     * @brief Set data bits
     *
     * @param bits Number of data bits (typically 7, 8, or 9)
     * @return Reference to this builder for chaining
     */
    constexpr UartBuilder& data_bits(u8 bits) {
        data_bits_ = bits;
        state_.has_data_bits = true;
        return *this;
    }

    /**
     * @brief Set stop bits
     *
     * @param bits Number of stop bits (typically 1 or 2)
     * @return Reference to this builder for chaining
     */
    constexpr UartBuilder& stop_bits(u8 bits) {
        stop_bits_ = bits;
        state_.has_stop_bits = true;
        return *this;
    }

    /**
     * @brief Enable flow control
     *
     * @param enable True to enable RTS/CTS flow control
     * @return Reference to this builder for chaining
     */
    constexpr UartBuilder& flow_control(bool enable = true) {
        flow_control_ = enable;
        return *this;
    }

    // ========================================================================
    // Preset Configuration Methods
    // ========================================================================

    /**
     * @brief Apply standard 8N1 configuration
     *
     * Sets 8 data bits, no parity, 1 stop bit.
     *
     * @return Reference to this builder for chaining
     */
    constexpr UartBuilder& standard_8n1() {
        data_bits(8);
        parity(UartParity::NONE);
        stop_bits(1);
        return *this;
    }

    /**
     * @brief Apply even parity configuration
     *
     * Sets 8 data bits, even parity, 1 stop bit.
     *
     * @return Reference to this builder for chaining
     */
    constexpr UartBuilder& standard_8e1() {
        data_bits(8);
        parity(UartParity::EVEN);
        stop_bits(1);
        return *this;
    }

    /**
     * @brief Apply odd parity configuration
     *
     * Sets 8 data bits, odd parity, 1 stop bit.
     *
     * @return Reference to this builder for chaining
     */
    constexpr UartBuilder& standard_8o1() {
        data_bits(8);
        parity(UartParity::ODD);
        stop_bits(1);
        return *this;
    }

    // ========================================================================
    // Validation and Initialization
    // ========================================================================

    /**
     * @brief Validate current configuration
     *
     * Checks if all required parameters are set.
     *
     * @return Result indicating success or validation error
     */
    Result<void, ErrorCode> validate() const {
        if (!state_.has_baudrate) {
            return Err(ErrorCode::InvalidParameter);
        }

        if (!state_.has_tx_pin && !state_.has_rx_pin) {
            return Err(ErrorCode::InvalidParameter);
        }

        // At least TX or RX must be configured
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        return Ok();
    }

    /**
     * @brief Initialize UART with current configuration
     *
     * Validates configuration and applies it to hardware.
     *
     * @return Result with configured UART or error
     */
    Result<FluentUartConfig<HardwarePolicy>, ErrorCode> initialize() const {
        auto validation = validate();
        if (!validation.is_ok()) {
            // Need to copy error to avoid reference issues
            ErrorCode error_copy = validation.error();
            return Err(std::move(error_copy));
        }

        return Ok(FluentUartConfig<HardwarePolicy>(
            PeriphId,
            tx_pin_id_,
            rx_pin_id_,
            baudrate_,
            parity_,
            data_bits_,
            stop_bits_,
            flow_control_,
            state_.has_tx_pin,
            state_.has_rx_pin
        ));
    }

    /**
     * @brief Get current builder state
     *
     * Useful for debugging and testing.
     *
     * @return Current builder state
     */
    constexpr BuilderState get_state() const {
        return state_;
    }

private:
    BaudRate baudrate_;
    UartParity parity_;
    u8 data_bits_;
    u8 stop_bits_;
    bool flow_control_;
    PinId tx_pin_id_;
    PinId rx_pin_id_;
    BuilderState state_;
};

}  // namespace alloy::hal
