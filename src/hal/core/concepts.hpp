/**
 * @file concepts.hpp
 * @brief C++20 Concepts for HAL Peripheral Validation
 *
 * Provides compile-time interface validation for all HAL peripherals with
 * clear, actionable error messages. Part of the modernize-peripheral-architecture
 * OpenSpec change.
 *
 * Design Principles:
 * - Concepts replace SFINAE for 10x better error messages
 * - Compile-time validation with zero runtime overhead
 * - Explicit type requirements for all peripheral interfaces
 * - Compatible with existing code (opt-in, non-breaking)
 *
 * @note Part of Phase 1: Core Concept Definitions
 * @see openspec/changes/modernize-peripheral-architecture/specs/concept-layer/spec.md
 */

#pragma once

#include <concepts>
#include <span>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

namespace alloy::hal::concepts {

using namespace alloy::core;

// ============================================================================
// Core Peripheral Concepts
// ============================================================================

/**
 * @brief Clock Platform Concept
 *
 * Validates that a type implements the clock platform interface.
 * All clock implementations must satisfy this concept.
 *
 * Requirements:
 * - initialize() to configure system clock
 * - enable_gpio_clocks() to enable GPIO peripheral clocks
 * - enable_uart_clock() to enable UART/USART peripheral clocks
 * - enable_spi_clock() to enable SPI peripheral clocks
 * - enable_i2c_clock() to enable I2C peripheral clocks
 *
 * Error messages will specify which method is missing.
 *
 * Example usage:
 * @code
 * template<ClockPlatform Clock>
 * void init_system(Clock& clock) {
 *     clock.initialize();  // Guaranteed to exist
 * }
 * @endcode
 */
template <typename T>
concept ClockPlatform = requires {
    // System clock initialization
    { T::initialize() } -> std::same_as<Result<void, ErrorCode>>;

    // Peripheral clock enable functions
    { T::enable_gpio_clocks() } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_uart_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_spi_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
    { T::enable_i2c_clock(u32{}) } -> std::same_as<Result<void, ErrorCode>>;
};

/**
 * @brief GPIO Pin Concept
 *
 * Validates that a type implements the complete GPIO pin interface.
 * All GPIO implementations must satisfy this concept.
 *
 * Requirements:
 * - set() method to set pin HIGH
 * - clear() method to set pin LOW
 * - toggle() method to toggle pin state
 * - read() method to read pin input value
 * - write(bool) method to write pin value
 * - setDirection() method to configure pin direction
 * - setDrive() method to configure drive mode
 * - setPull() method to configure pull resistors
 * - isOutput() method to check if pin is output
 *
 * Error messages will specify which method is missing.
 *
 * Example usage:
 * @code
 * template<GpioPin Pin>
 * void blink(Pin& pin) {
 *     pin.toggle();  // Guaranteed to exist
 * }
 * @endcode
 */
template <typename T>
concept GpioPin = requires(T pin, const T const_pin, PinDirection direction, PinDrive drive,
                            PinPull pull, bool value) {
    // State manipulation methods
    { pin.set() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.clear() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.toggle() } -> std::same_as<Result<void, ErrorCode>>;
    { pin.write(value) } -> std::same_as<Result<void, ErrorCode>>;

    // State reading
    { const_pin.read() } -> std::same_as<Result<bool, ErrorCode>>;
    { const_pin.isOutput() } -> std::same_as<Result<bool, ErrorCode>>;

    // Configuration methods
    { pin.setDirection(direction) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setDrive(drive) } -> std::same_as<Result<void, ErrorCode>>;
    { pin.setPull(pull) } -> std::same_as<Result<void, ErrorCode>>;

    // Compile-time metadata
    requires requires { T::port_base; };
    requires requires { T::pin_number; };
    requires requires { T::pin_mask; };
};

/**
 * @brief UART Peripheral Concept
 *
 * Validates that a type implements the UART peripheral interface.
 * All UART implementations must satisfy this concept.
 *
 * Requirements:
 * - read_byte() to read a single byte
 * - write_byte() to write a single byte
 * - available() to check received data count
 * - configure() to set UART parameters
 *
 * Error messages will specify which method is missing or has wrong signature.
 *
 * Example usage:
 * @code
 * template<UartPeripheral Uart>
 * void send_hello(Uart& uart) {
 *     uart.write_byte('H');
 * }
 * @endcode
 */
template <typename T>
concept UartPeripheral = requires(T device, const T const_device, u8 byte) {
    // Read/write operations
    { device.read_byte() } -> std::same_as<Result<u8, ErrorCode>>;
    { device.write_byte(byte) } -> std::same_as<Result<void, ErrorCode>>;

    // Status queries
    { const_device.available() } -> std::convertible_to<usize>;

    // Configuration - requires UartConfig from interface/uart.hpp
    // Note: We don't check configure() signature here as it depends on UartConfig
    // which may vary. The interface file already has UartDevice concept for that.
};

/**
 * @brief SPI Peripheral Concept
 *
 * Validates that a type implements the SPI master interface.
 * All SPI implementations must satisfy this concept.
 *
 * Requirements:
 * - transfer() for full-duplex communication
 * - transmit() for TX-only communication
 * - receive() for RX-only communication
 * - is_busy() to check transfer status
 *
 * Error messages will specify which method is missing or has wrong signature.
 *
 * Example usage:
 * @code
 * template<SpiPeripheral Spi>
 * void write_register(Spi& spi, u8 reg, u8 value) {
 *     u8 data[2] = {reg, value};
 *     spi.transmit(std::span(data, 2));
 * }
 * @endcode
 */
template <typename T>
concept SpiPeripheral = requires(T device, const T const_device, std::span<u8> rx_buffer,
                                  std::span<const u8> tx_buffer) {
    // Transfer operations
    { device.transfer(tx_buffer, rx_buffer) } -> std::same_as<Result<void, ErrorCode>>;
    { device.transmit(tx_buffer) } -> std::same_as<Result<void, ErrorCode>>;
    { device.receive(rx_buffer) } -> std::same_as<Result<void, ErrorCode>>;

    // Status queries
    { const_device.is_busy() } -> std::convertible_to<bool>;

    // Configuration - similar to UART, specific config struct not checked here
};

/**
 * @brief Timer Peripheral Concept
 *
 * Validates that a type implements the timer peripheral interface.
 * All timer implementations must satisfy this concept.
 *
 * Requirements:
 * - start() to start the timer
 * - stop() to stop the timer
 * - get_count() to read current count
 * - set_period() to configure period
 * - is_running() to check if timer is active
 *
 * Error messages will specify which method is missing.
 */
template <typename T>
concept TimerPeripheral = requires(T timer, const T const_timer, u32 period) {
    // Control methods
    { timer.start() } -> std::same_as<Result<void, ErrorCode>>;
    { timer.stop() } -> std::same_as<Result<void, ErrorCode>>;

    // Configuration
    { timer.set_period(period) } -> std::same_as<Result<void, ErrorCode>>;

    // Status and reading
    { const_timer.get_count() } -> std::same_as<Result<u32, ErrorCode>>;
    { const_timer.is_running() } -> std::convertible_to<bool>;
};

/**
 * @brief ADC Peripheral Concept
 *
 * Validates that a type implements the ADC peripheral interface.
 *
 * Requirements:
 * - read() to perform a single conversion
 * - start_conversion() to begin conversion
 * - is_conversion_complete() to check status
 *
 * Error messages will specify which method is missing.
 */
template <typename T>
concept AdcPeripheral = requires(T adc, const T const_adc) {
    // Conversion operations
    { adc.read() } -> std::same_as<Result<u16, ErrorCode>>;
    { adc.start_conversion() } -> std::same_as<Result<void, ErrorCode>>;

    // Status queries
    { const_adc.is_conversion_complete() } -> std::convertible_to<bool>;
};

/**
 * @brief PWM Peripheral Concept
 *
 * Validates that a type implements the PWM peripheral interface.
 *
 * Requirements:
 * - set_duty_cycle() to configure duty cycle
 * - set_frequency() to configure PWM frequency
 * - start() to start PWM output
 * - stop() to stop PWM output
 *
 * Error messages will specify which method is missing.
 */
template <typename T>
concept PwmPeripheral = requires(T pwm, u32 frequency, u32 duty_cycle) {
    // Configuration
    { pwm.set_frequency(frequency) } -> std::same_as<Result<void, ErrorCode>>;
    { pwm.set_duty_cycle(duty_cycle) } -> std::same_as<Result<void, ErrorCode>>;

    // Control
    { pwm.start() } -> std::same_as<Result<void, ErrorCode>>;
    { pwm.stop() } -> std::same_as<Result<void, ErrorCode>>;
};

// ============================================================================
// Peripheral Capability Concepts
// ============================================================================

/**
 * @brief Interrupt Capable Concept
 *
 * Validates that a peripheral supports interrupt configuration.
 *
 * Requirements:
 * - enable_interrupt() to enable peripheral interrupt
 * - disable_interrupt() to disable peripheral interrupt
 * - clear_interrupt() to clear interrupt flag
 */
template <typename T>
concept InterruptCapable = requires(T peripheral) {
    { peripheral.enable_interrupt() } -> std::same_as<Result<void, ErrorCode>>;
    { peripheral.disable_interrupt() } -> std::same_as<Result<void, ErrorCode>>;
    { peripheral.clear_interrupt() } -> std::same_as<Result<void, ErrorCode>>;
};

/**
 * @brief DMA Capable Concept
 *
 * Validates that a peripheral supports DMA transfers.
 *
 * Requirements:
 * - enable_dma() to enable DMA mode
 * - disable_dma() to disable DMA mode
 * - get_data_register_address() to get peripheral data register address for DMA
 */
template <typename T>
concept DmaCapable = requires(T peripheral) {
    { peripheral.enable_dma() } -> std::same_as<Result<void, ErrorCode>>;
    { peripheral.disable_dma() } -> std::same_as<Result<void, ErrorCode>>;
    { peripheral.get_data_register_address() } -> std::same_as<usize>;
};

// ============================================================================
// Configuration Validation Concepts
// ============================================================================

/**
 * @brief Valid Configuration Concept
 *
 * Validates that a configuration struct is complete and has valid values.
 * This is a meta-concept that checks for a validation method.
 *
 * Requirements:
 * - is_valid() constexpr method returning bool
 * - error_message() constexpr method returning error description
 *
 * Example usage:
 * @code
 * constexpr UsartConfig cfg { ... };
 * static_assert(cfg.is_valid(), cfg.error_message());
 * @endcode
 */
template <typename T>
concept ValidConfiguration = requires(const T config) {
    { config.is_valid() } -> std::same_as<bool>;
    { config.error_message() } -> std::convertible_to<const char*>;
};

// ============================================================================
// Compile-Time Tests
// ============================================================================

// These static_asserts ensure the concept definitions themselves are valid.
// They will be uncommented once we have concrete implementations to test.

// Example test (uncomment when implementations exist):
// static_assert(GpioPin<alloy::hal::same70::GpioPin<0x400E0E00, 0>>);
// static_assert(!GpioPin<int>, "int should not satisfy GpioPin concept");

}  // namespace alloy::hal::concepts
