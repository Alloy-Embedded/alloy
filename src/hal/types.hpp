/**
 * @file types.hpp
 * @brief Common HAL types used across all platforms
 *
 * This file defines platform-agnostic types for hardware abstraction layer.
 * These types are used by all platform implementations (SAME70, Linux, ESP32, etc.)
 *
 * Design Principles:
 * - Platform-agnostic: Works on ARM, ESP32, Linux, etc.
 * - Zero overhead: Uses enums and type aliases, no virtual functions
 * - Type-safe: Strong typing prevents errors (e.g., can't pass Parity to setBaudrate)
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 * @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
 */

#pragma once

#include <cstdint>

#include "core/types.hpp"

namespace alloy::hal {

// ============================================================================
// UART Types
// ============================================================================

/**
 * @brief Standard UART baud rates
 *
 * Defines commonly used baud rates for UART communication.
 * Platform implementations must support at least the standard rates.
 *
 * @note Enum values are the actual baud rate numbers for easy debugging
 */
enum class Baudrate : uint32_t {
    e9600 = 9600,
    e19200 = 19200,
    e38400 = 38400,
    e57600 = 57600,
    e115200 = 115200,
    e230400 = 230400,
    e460800 = 460800,
    e921600 = 921600,
};

/**
 * @brief UART parity modes
 */
enum class Parity : uint8_t {
    None = 0,   ///< No parity bit
    Even = 1,   ///< Even parity
    Odd = 2,    ///< Odd parity
    Mark = 3,   ///< Mark parity (always 1)
    Space = 4,  ///< Space parity (always 0)
};

/**
 * @brief UART stop bits
 */
enum class StopBits : uint8_t {
    One = 0,         ///< 1 stop bit
    OneAndHalf = 1,  ///< 1.5 stop bits
    Two = 2,         ///< 2 stop bits
};

/**
 * @brief UART data bits
 */
enum class DataBits : uint8_t {
    Five = 5,   ///< 5 data bits
    Six = 6,    ///< 6 data bits
    Seven = 7,  ///< 7 data bits
    Eight = 8,  ///< 8 data bits
};

/**
 * @brief UART flow control modes
 */
enum class FlowControl : uint8_t {
    None = 0,     ///< No flow control
    RtsCts = 1,   ///< Hardware flow control (RTS/CTS)
    XonXoff = 2,  ///< Software flow control (XON/XOFF)
};

/**
 * @brief UART configuration structure
 *
 * Groups all UART configuration parameters into a single structure.
 * Can be used to configure UART in a single call.
 *
 * Example:
 * @code
 * UartConfig config {
 *     .baudrate = Baudrate::e115200,
 *     .data_bits = DataBits::Eight,
 *     .parity = Parity::None,
 *     .stop_bits = StopBits::One,
 *     .flow_control = FlowControl::None
 * };
 * uart.configure(config);
 * @endcode
 */
struct UartConfig {
    Baudrate baudrate = Baudrate::e115200;
    DataBits data_bits = DataBits::Eight;
    Parity parity = Parity::None;
    StopBits stop_bits = StopBits::One;
    FlowControl flow_control = FlowControl::None;
};

// ============================================================================
// GPIO Types
// ============================================================================

/**
 * @brief GPIO pin direction
 */
enum class PinDirection : uint8_t {
    Input = 0,   ///< Input pin (reads external signal)
    Output = 1,  ///< Output pin (drives external signal)
};

/**
 * @brief GPIO pin state (logical level)
 */
enum class PinState : uint8_t {
    Low = 0,   ///< Logic low (0V or GND)
    High = 1,  ///< Logic high (3.3V or 5V)
};

/**
 * @brief GPIO pull resistor configuration
 */
enum class PinPull : uint8_t {
    None = 0,      ///< No pull resistor (floating)
    PullUp = 1,    ///< Pull-up resistor enabled
    PullDown = 2,  ///< Pull-down resistor enabled
};

/**
 * @brief GPIO output drive mode
 */
enum class PinDrive : uint8_t {
    PushPull = 0,   ///< Push-pull output (actively drives high and low)
    OpenDrain = 1,  ///< Open-drain output (only drives low, high is floating)
};

/**
 * @brief GPIO interrupt trigger mode
 */
enum class InterruptMode : uint8_t {
    None = 0,         ///< No interrupt
    RisingEdge = 1,   ///< Trigger on rising edge (low → high)
    FallingEdge = 2,  ///< Trigger on falling edge (high → low)
    BothEdges = 3,    ///< Trigger on both edges
    LowLevel = 4,     ///< Trigger when pin is low
    HighLevel = 5,    ///< Trigger when pin is high
};

/**
 * @brief GPIO pin configuration structure
 *
 * Groups all GPIO configuration parameters into a single structure.
 *
 * Example:
 * @code
 * GpioConfig config {
 *     .direction = PinDirection::Output,
 *     .drive = PinDrive::PushPull,
 *     .pull = PinPull::None,
 *     .initial_state = PinState::Low
 * };
 * gpio.configure(config);
 * @endcode
 */
struct GpioConfig {
    PinDirection direction = PinDirection::Input;
    PinDrive drive = PinDrive::PushPull;
    PinPull pull = PinPull::None;
    PinState initial_state = PinState::Low;
};

// ============================================================================
// I2C Types
// ============================================================================

// NOTE: I2C types moved to hal/interface/i2c.hpp to avoid redefinition
// Use the types from hal/interface/i2c.hpp instead:
// - I2cAddressing (SevenBit, TenBit)
// - I2cSpeed (Standard, Fast, FastPlus, HighSpeed)
// - I2cConfig

// ============================================================================
// SPI Types
// ============================================================================

// NOTE: SPI types moved to hal/interface/spi.hpp to avoid redefinition
// Use the types from hal/interface/spi.hpp instead:
// - SpiMode (Mode0, Mode1, Mode2, Mode3)
// - SpiBitOrder (MsbFirst, LsbFirst)
// - SpiDataSize (Bits8, Bits16)
// - SpiConfig

/**
 * @brief SPI chip select mode
 */
enum class SpiCsMode : uint8_t {
    Hardware = 0,  ///< Hardware-controlled chip select
    Software = 1,  ///< Software-controlled chip select (manual toggle)
};

// ============================================================================
// Timer/PWM Types
// ============================================================================

/**
 * @brief PWM polarity
 */
enum class PwmPolarity : uint8_t {
    Normal = 0,    ///< Normal polarity (high = active)
    Inverted = 1,  ///< Inverted polarity (low = active)
};

/**
 * @brief PWM alignment mode
 */
enum class PwmAlignment : uint8_t {
    Edge = 0,    ///< Edge-aligned PWM
    Center = 1,  ///< Center-aligned PWM
};

/**
 * @brief PWM configuration structure
 */
struct PwmConfig {
    uint32_t frequency_hz = 1000;     ///< PWM frequency in Hz (default 1 kHz)
    uint8_t duty_cycle_percent = 50;  ///< Duty cycle 0-100% (default 50%)
    PwmPolarity polarity = PwmPolarity::Normal;
    PwmAlignment alignment = PwmAlignment::Edge;
};

// ============================================================================
// ADC Types
// ============================================================================

// NOTE: ADC types moved to hal/interface/adc.hpp to avoid redefinition
// Use the types from hal/interface/adc.hpp instead:
// - AdcResolution (Bits6, Bits8, Bits10, Bits12, Bits14, Bits16)
// - AdcReference (Internal, External, Vdd)
// - AdcSampleTime (Cycles1_5, Cycles7_5, etc.)
// - AdcChannel (Channel0-Channel18)
// - AdcConfig

}  // namespace alloy::hal
