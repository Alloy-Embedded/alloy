/**
 * @file signals.hpp
 * @brief Signal Routing Infrastructure for Peripheral Interconnections
 *
 * Provides type-safe signal routing between peripherals (GPIO↔UART, ADC↔DMA, Timer↔ADC)
 * with compile-time validation. Enables explicit configuration of alternate functions,
 * DMA connections, and other peripheral-to-peripheral signals.
 *
 * Design Principles:
 * - Strongly typed signals for compile-time validation
 * - Explicit routing (no implicit connections)
 * - Clear error messages when incompatible signals are connected
 * - Zero runtime overhead (all checks at compile-time)
 * - Generated from SVD for accuracy
 *
 * @note Part of Phase 1: Signal Infrastructure
 * @see openspec/changes/modernize-peripheral-architecture/specs/signal-routing/spec.md
 */

#pragma once

#include <array>
#include <concepts>

#include "core/types.hpp"
#include "hal/types.hpp"

namespace alloy::hal::signals {

using namespace alloy::core;

// ============================================================================
// Core Signal Types
// ============================================================================

/**
 * @brief Peripheral identifier
 *
 * Unique identifier for each peripheral instance.
 * Values are platform-specific and will be generated from SVD.
 */
enum class PeripheralId : u16 {
    // USART/UART
    USART0 = 0,
    USART1 = 1,
    USART2 = 2,
    USART3 = 3,
    UART0 = 5,

    // SPI
    SPI0 = 10,
    SPI1 = 11,
    SPI2 = 12,

    // I2C/TWI
    I2C1 = 20,
    I2C2 = 21,
    TWI0 = 22,
    TWI1 = 23,

    // ADC/DAC
    ADC0 = 30,
    ADC1 = 31,
    DAC = 35,

    // Timer/Counter
    TC0 = 40,
    TC1 = 41,
    TC2 = 42,
    TIMER1 = 45,
    TIMER2 = 46,

    // PWM
    PWM0 = 50,
    PWM1 = 51,

    // DMA
    DMA1 = 60,

    // Network
    GMAC = 70,

    // Placeholder for SAME70 vendor-specific
    SAME70 = 1000,

    // ... more peripherals to be generated from SVD
};

/**
 * @brief Signal type classification
 *
 * Categorizes the type of signal being routed.
 */
enum class SignalType : u8 {
    TX,           // Transmit data
    RX,           // Receive data
    CLK,          // Clock signal
    CS,           // Chip select
    MOSI,         // SPI Master Out Slave In
    MISO,         // SPI Master In Slave Out
    SCL,          // I2C Clock
    SDA,          // I2C Data
    TRIGGER,      // Timer/ADC trigger
    DATA,         // Generic data signal
    REQUEST,      // DMA request
    CUSTOM        // Platform-specific signal
};

/**
 * @brief Pin identifier
 *
 * Unique identifier for each GPIO pin.
 * Format: Port letter + pin number (e.g., PA9 = port A, pin 9)
 */
enum class PinId : u16 {
    // Port A
    PA0 = 0,   PA1,  PA2,  PA3,  PA4,  PA5,  PA6,  PA7,
    PA8,  PA9,  PA10, PA11, PA12, PA13, PA14, PA15,
    PA16, PA17, PA18, PA19, PA20, PA21, PA22, PA23,
    PA24, PA25, PA26, PA27, PA28, PA29, PA30, PA31,

    // Port B
    PB0 = 100, PB1,  PB2,  PB3,  PB4,  PB5,  PB6,  PB7,
    PB8,  PB9,  PB10, PB11, PB12, PB13, PB14, PB15,
    PB16, PB17, PB18, PB19, PB20, PB21, PB22, PB23,
    PB24, PB25, PB26, PB27, PB28, PB29, PB30, PB31,

    // Port C
    PC0 = 200, PC1,  PC2,  PC3,  PC4,  PC5,  PC6,  PC7,
    PC8,  PC9,  PC10, PC11, PC12, PC13, PC14, PC15,
    PC16, PC17, PC18, PC19, PC20, PC21, PC22, PC23,
    PC24, PC25, PC26, PC27, PC28, PC29, PC30, PC31,

    // Port D
    PD0 = 300, PD1,  PD2,  PD3,  PD4,  PD5,  PD6,  PD7,
    PD8,  PD9,  PD10, PD11, PD12, PD13, PD14, PD15,
    PD16, PD17, PD18, PD19, PD20, PD21, PD22, PD23,
    PD24, PD25, PD26, PD27, PD28, PD29, PD30, PD31,

    // Port E
    PE0 = 400, PE1,  PE2,  PE3,  PE4,  PE5,  PE6,  PE7,
    PE8,  PE9,  PE10, PE11, PE12, PE13, PE14, PE15,
    PE16, PE17, PE18, PE19, PE20, PE21, PE22, PE23,
    PE24, PE25, PE26, PE27, PE28, PE29, PE30, PE31,
};

/**
 * @brief Alternate function identifier
 *
 * Represents alternate function numbers for pin multiplexing.
 * Platform-specific (e.g., STM32 uses AF0-AF15, SAME70 uses peripheral A-D).
 */
enum class AlternateFunction : u8 {
    // STM32-style alternate functions
    AF0 = 0,  AF1,  AF2,  AF3,  AF4,  AF5,  AF6,  AF7,
    AF8,  AF9,  AF10, AF11, AF12, AF13, AF14, AF15,

    // SAME70-style peripheral functions
    PERIPH_A = 100,
    PERIPH_B = 101,
    PERIPH_C = 102,
    PERIPH_D = 103,
};

/**
 * @brief DMA request type
 *
 * Specifies the type of DMA request from a peripheral.
 */
enum class DmaRequestType : u8 {
    TX,    // Transmit request
    RX,    // Receive request
    DATA,  // Generic data transfer
};

// ============================================================================
// Signal Definitions
// ============================================================================

/**
 * @brief Pin definition for signal routing
 *
 * Associates a GPIO pin with its alternate function for a specific peripheral signal.
 */
struct PinDef {
    PinId pin;
    AlternateFunction af;

    constexpr PinDef(PinId p, AlternateFunction a) : pin(p), af(a) {}
};

/**
 * @brief Peripheral signal definition (to be generated from SVD)
 *
 * Base template for all peripheral signals. Specializations will be
 * generated from SVD files with platform-specific pin mappings.
 *
 * Example specialization (generated):
 * @code
 * template<>
 * struct PeripheralSignal<PeripheralId::USART1, SignalType::TX> {
 *     static constexpr PeripheralId peripheral = PeripheralId::USART1;
 *     static constexpr SignalType type = SignalType::TX;
 *     static constexpr std::array compatible_pins = {
 *         PinDef{PinId::PA9, AlternateFunction::AF7},
 *         PinDef{PinId::PA15, AlternateFunction::AF7},
 *         PinDef{PinId::PB6, AlternateFunction::AF7}
 *     };
 * };
 * @endcode
 */
template <PeripheralId Peripheral, SignalType Signal>
struct PeripheralSignal {
    static constexpr PeripheralId peripheral = Peripheral;
    static constexpr SignalType type = Signal;
    // compatible_pins array to be defined in specializations (generated from SVD)
};

/**
 * @brief DMA peripheral definition (to be generated from SVD)
 *
 * Defines which peripherals a DMA channel/stream supports.
 *
 * Example specialization (generated):
 * @code
 * struct DmaChannel {
 *     PeripheralId peripheral;
 *     DmaRequestType request_type;
 * };
 *
 * template<>
 * struct DmaCompatibility<PeripheralId::DMA1, 7> {  // DMA1 Stream 7
 *     static constexpr std::array supported_peripherals = {
 *         DmaChannel{PeripheralId::USART1, DmaRequestType::TX},
 *         DmaChannel{PeripheralId::SPI1, DmaRequestType::TX},
 *     };
 * };
 * @endcode
 */
template <PeripheralId DmaPeripheral, u8 ChannelNumber>
struct DmaCompatibility {
    static constexpr PeripheralId peripheral = DmaPeripheral;
    static constexpr u8 channel = ChannelNumber;
    // supported_peripherals array to be defined in specializations (generated from SVD)
};

// ============================================================================
// Compile-Time Validation Helpers
// ============================================================================

/**
 * @brief Check if a pin supports a specific peripheral signal
 *
 * Compile-time helper to validate pin compatibility with a signal.
 *
 * @tparam Signal The peripheral signal type
 * @param pin The pin to check
 * @return true if pin is compatible with signal, false otherwise
 */
template <typename Signal>
constexpr bool pin_supports_signal(PinId pin) {
    if constexpr (requires { Signal::compatible_pins; }) {
        for (const auto& pin_def : Signal::compatible_pins) {
            if (pin_def.pin == pin) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Get alternate function for a pin/signal combination
 *
 * Returns the alternate function number needed to route a signal to a pin.
 *
 * @tparam Signal The peripheral signal type
 * @param pin The pin to query
 * @return AlternateFunction if valid, otherwise AF0
 */
template <typename Signal>
constexpr AlternateFunction get_alternate_function(PinId pin) {
    if constexpr (requires { Signal::compatible_pins; }) {
        for (const auto& pin_def : Signal::compatible_pins) {
            if (pin_def.pin == pin) {
                return pin_def.af;
            }
        }
    }
    return AlternateFunction::AF0;  // Invalid/not found
}

// ============================================================================
// Concepts for Signal Validation
// ============================================================================

/**
 * @brief Concept to check if a pin supports a specific signal
 *
 * Used in template constraints to enforce valid pin/signal combinations.
 *
 * Example usage:
 * @code
 * template<PinId Pin, typename Signal>
 *     requires SupportsSignal<Pin, Signal>
 * void configure_pin() {
 *     // Pin is guaranteed to support Signal
 * }
 * @endcode
 */
template <PinId Pin, typename Signal>
concept SupportsSignal = pin_supports_signal<Signal>(Pin);

/**
 * @brief Concept to check if a DMA channel supports a peripheral
 *
 * Used in template constraints to enforce valid DMA connections.
 *
 * Example usage:
 * @code
 * template<typename DmaChannel, PeripheralId Peripheral>
 *     requires DmaSupportsPeripheral<DmaChannel, Peripheral>
 * void configure_dma() {
 *     // DMA channel is guaranteed to support Peripheral
 * }
 * @endcode
 */
template <typename DmaChannel, PeripheralId Peripheral>
concept DmaSupportsPeripheral = requires {
    // This will be implemented once DMA compatibility tables are generated
    requires std::same_as<decltype(DmaChannel::peripheral), const PeripheralId>;
};

// ============================================================================
// Signal Connection API (Phase 3.2: Pin→Signal Connection)
// ============================================================================

/**
 * @brief Signal connection result with detailed error information
 *
 * Represents the result of validating a signal connection.
 * Provides error messages and suggestions when validation fails.
 */
struct PinSignalConnection {
    bool valid;
    PinId pin;
    const char* signal_name;
    const char* error_message;
    const char* suggestion;

    constexpr bool is_valid() const { return valid; }
    constexpr const char* error() const { return error_message; }
    constexpr const char* hint() const { return suggestion; }
    constexpr PinId get_pin() const { return pin; }
};

/**
 * @brief Validate a pin-to-signal connection at compile-time
 *
 * Checks if a pin can be connected to a specific signal and provides
 * detailed error messages if not.
 *
 * @tparam Signal The peripheral signal type
 * @param pin The pin to validate
 * @return SignalConnection with validation result
 *
 * Example:
 * @code
 * constexpr auto conn = validate_connection<Usart0RxSignal>(PinId::PD4);
 * static_assert(conn.is_valid(), conn.error());
 * @endcode
 */
template <typename Signal>
constexpr PinSignalConnection validate_connection(PinId pin) {
    if (pin_supports_signal<Signal>(pin)) {
        return PinSignalConnection{
            .valid = true,
            .pin = pin,
            .signal_name = "Unknown",  // Would need constexpr string from Signal
            .error_message = "",
            .suggestion = ""
        };
    }
    return PinSignalConnection{
        .valid = false,
        .pin = pin,
        .signal_name = "Unknown",
        .error_message = "Pin does not support this signal",
        .suggestion = "Check signal routing tables for compatible pins"
    };
}

/**
 * @brief Connect a pin to a signal with compile-time validation
 *
 * This is a convenience function that validates the connection and
 * provides helpful error messages if invalid.
 *
 * @tparam Signal The peripheral signal type
 * @tparam Pin The GPIO pin type
 * @return PinSignalConnection with validation result
 *
 * Example:
 * @code
 * using TxPin = GpioPin<PIOD_BASE, 4>;
 * constexpr auto conn = connect<Usart0TxSignal, TxPin>();
 * static_assert(conn.is_valid(), "Invalid pin for USART0 TX");
 * @endcode
 */
template <typename Signal, typename Pin>
constexpr PinSignalConnection connect() {
    // Get pin's PinId
    constexpr PinId pin_id = Pin::get_pin_id();

    // Validate connection
    return validate_connection<Signal>(pin_id);
}

/**
 * @brief Helper to get list of compatible pins for a signal
 *
 * Returns a compile-time array of compatible pins for error messages.
 *
 * @tparam Signal The peripheral signal type
 * @return Array of compatible PinIds
 */
template <typename Signal>
constexpr auto get_compatible_pins() {
    if constexpr (requires { Signal::compatible_pins; }) {
        // Extract just the PinIds
        constexpr auto size = Signal::compatible_pins.size();
        std::array<PinId, size> pins{};
        for (size_t i = 0; i < size; ++i) {
            pins[i] = Signal::compatible_pins[i].pin;
        }
        return pins;
    } else {
        return std::array<PinId, 0>{};
    }
}

}  // namespace alloy::hal::signals
