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

/**
 * @brief I2C addressing mode
 */
enum class I2cAddressing : uint8_t {
    SevenBit = 0,  ///< 7-bit addressing (default)
    TenBit = 1,    ///< 10-bit addressing
};

/**
 * @brief I2C clock speed presets
 */
enum class I2cSpeed : uint32_t {
    Standard = 100000,    ///< 100 kHz (standard mode)
    Fast = 400000,        ///< 400 kHz (fast mode)
    FastPlus = 1000000,   ///< 1 MHz (fast mode plus)
    HighSpeed = 3400000,  ///< 3.4 MHz (high speed mode)
};

/**
 * @brief I2C configuration structure
 */
struct I2cConfig {
    I2cSpeed speed = I2cSpeed::Fast;
    I2cAddressing addressing = I2cAddressing::SevenBit;
    bool enable_general_call = false;  ///< Respond to general call address (0x00)
    uint8_t own_address = 0x00;        ///< Own address (for slave mode)
};

// ============================================================================
// SPI Types
// ============================================================================

/**
 * @brief SPI mode (clock polarity and phase)
 *
 * SPI modes define the clock polarity (CPOL) and phase (CPHA):
 * - Mode 0: CPOL=0, CPHA=0 (clock idle low, sample on rising edge)
 * - Mode 1: CPOL=0, CPHA=1 (clock idle low, sample on falling edge)
 * - Mode 2: CPOL=1, CPHA=0 (clock idle high, sample on falling edge)
 * - Mode 3: CPOL=1, CPHA=1 (clock idle high, sample on rising edge)
 */
enum class SpiMode : uint8_t {
    Mode0 = 0,  ///< CPOL=0, CPHA=0
    Mode1 = 1,  ///< CPOL=0, CPHA=1
    Mode2 = 2,  ///< CPOL=1, CPHA=0
    Mode3 = 3,  ///< CPOL=1, CPHA=1
};

/**
 * @brief SPI bit order
 */
enum class SpiBitOrder : uint8_t {
    MsbFirst = 0,  ///< Most significant bit first (default)
    LsbFirst = 1,  ///< Least significant bit first
};

/**
 * @brief SPI data bits per transfer
 */
enum class SpiDataBits : uint8_t {
    Bits8 = 8,    ///< 8 bits per transfer (default)
    Bits16 = 16,  ///< 16 bits per transfer
};

/**
 * @brief SPI chip select mode
 */
enum class SpiCsMode : uint8_t {
    Hardware = 0,  ///< Hardware-controlled chip select
    Software = 1,  ///< Software-controlled chip select (manual toggle)
};

/**
 * @brief SPI configuration structure
 */
struct SpiConfig {
    uint32_t clock_speed = 1000000;  ///< SPI clock speed in Hz (default 1 MHz)
    SpiMode mode = SpiMode::Mode0;
    SpiBitOrder bit_order = SpiBitOrder::MsbFirst;
    SpiDataBits data_bits = SpiDataBits::Bits8;
    SpiCsMode cs_mode = SpiCsMode::Hardware;
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

/**
 * @brief ADC resolution
 */
enum class AdcResolution : uint8_t {
    Bits8 = 8,    ///< 8-bit resolution (256 levels)
    Bits10 = 10,  ///< 10-bit resolution (1024 levels)
    Bits12 = 12,  ///< 12-bit resolution (4096 levels)
    Bits16 = 16,  ///< 16-bit resolution (65536 levels)
};

/**
 * @brief ADC reference voltage source
 */
enum class AdcReference : uint8_t {
    Internal = 0,  ///< Internal reference voltage
    External = 1,  ///< External reference voltage (VREF pin)
    Vcc = 2,       ///< VCC/VDD as reference
};

/**
 * @brief ADC sample time (affects conversion accuracy vs speed)
 */
enum class AdcSampleTime : uint8_t {
    Cycles1_5 = 0,
    Cycles7_5 = 1,
    Cycles13_5 = 2,
    Cycles28_5 = 3,
    Cycles41_5 = 4,
    Cycles55_5 = 5,
    Cycles71_5 = 6,
    Cycles239_5 = 7,
};

/**
 * @brief ADC configuration structure
 */
struct AdcConfig {
    AdcResolution resolution = AdcResolution::Bits12;
    AdcReference reference = AdcReference::Internal;
    AdcSampleTime sample_time = AdcSampleTime::Cycles28_5;
};

}  // namespace alloy::hal
