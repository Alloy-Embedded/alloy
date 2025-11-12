/**
 * @file same70_signals.hpp
 * @brief Signal Routing Tables for SAME70
 *
 * Auto-generated signal routing tables for peripheral signal validation.
 * Part of Phase 2: Signal Metadata Generation.
 *
 * Generated from: same70_pin_functions.py
 * Generator: generate_signal_tables.py
 *
 * @note Part of modernize-peripheral-architecture OpenSpec change
 * @see openspec/changes/modernize-peripheral-architecture/specs/signal-routing/spec.md
 */

#pragma once

#include "hal/signals.hpp"

namespace alloy::hal::atmel::same70 {

using namespace alloy::hal::signals;

// ============================================================================
// Peripheral Signal Specializations
// ============================================================================

// ADC Signals
// ------------------------------------------------------------------------------

/**
 * @brief ADC0 IN signal
 * Compatible pins: PA8, PB0
 */
struct ADC0INSignal {
    static constexpr PeripheralId peripheral = PeripheralId::ADC0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA8, AlternateFunction::PERIPH_C},
        PinDef{PinId::PB0, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief ADC1 IN signal
 * Compatible pins: PB1
 */
struct ADC1INSignal {
    static constexpr PeripheralId peripheral = PeripheralId::ADC1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB1, AlternateFunction::PERIPH_A}
    };
};

// DAC Signals
// ------------------------------------------------------------------------------

/**
 * @brief DAC TRIGGER signal
 * Compatible pins: PA2
 */
struct DACTRIGGERSignal {
    static constexpr PeripheralId peripheral = PeripheralId::DAC;
    static constexpr SignalType type = SignalType::TRIGGER;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA2, AlternateFunction::PERIPH_C}
    };
};

// GMAC Signals
// ------------------------------------------------------------------------------

/**
 * @brief USART0 RX signal
 * Compatible pins: PD4
 */
struct USART0RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART0;
    static constexpr SignalType type = SignalType::RX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD4, AlternateFunction::PERIPH_A}
    };
};

// PWM Signals
// ------------------------------------------------------------------------------

/**
 * @brief PWM0 HIGH signal
 * Compatible pins: PA0, PA1, PA11, PA12, PA2, PA30, PA7, PB0, PB1, PB4, PC0, PC1, PC12, PC13, PC14, PC15, PC2, PC3, PD0, PD1, PD2, PD24, PD3, PE0, PE1, PE2, PE3
 */
struct PWM0HIGHSignal {
    static constexpr PeripheralId peripheral = PeripheralId::PWM0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA0, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA1, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA11, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA12, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA2, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA30, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA7, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB0, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB1, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB4, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC0, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC1, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC12, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC13, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC14, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC15, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC2, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC3, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD0, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD1, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD2, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD24, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD3, AlternateFunction::PERIPH_B},
        PinDef{PinId::PE0, AlternateFunction::PERIPH_B},
        PinDef{PinId::PE1, AlternateFunction::PERIPH_B},
        PinDef{PinId::PE2, AlternateFunction::PERIPH_B},
        PinDef{PinId::PE3, AlternateFunction::PERIPH_B}
    };
};

/**
 * @brief PWM0 LOW signal
 * Compatible pins: PA8, PB5, PC10, PC11, PC8, PC9, PD4, PD5, PD6, PD7, PE4, PE5
 */
struct PWM0LOWSignal {
    static constexpr PeripheralId peripheral = PeripheralId::PWM0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA8, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB5, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC10, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC11, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC8, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC9, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD4, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD5, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD6, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD7, AlternateFunction::PERIPH_B},
        PinDef{PinId::PE4, AlternateFunction::PERIPH_B},
        PinDef{PinId::PE5, AlternateFunction::PERIPH_B}
    };
};

// SPI Signals
// ------------------------------------------------------------------------------

/**
 * @brief SPI0 CLK signal
 * Compatible pins: PA22, PC18
 */
struct SPI0CLKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::SPI0;
    static constexpr SignalType type = SignalType::CLK;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA22, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC18, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief SPI0 CS signal
 * Compatible pins: PA25, PA30, PA31, PC19, PC4, PD28
 */
struct SPI0CSSignal {
    static constexpr PeripheralId peripheral = PeripheralId::SPI0;
    static constexpr SignalType type = SignalType::CS;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA25, AlternateFunction::PERIPH_C},
        PinDef{PinId::PA30, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA31, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC19, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC4, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD28, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief SPI0 MISO signal
 * Compatible pins: PA24, PC16
 */
struct SPI0MISOSignal {
    static constexpr PeripheralId peripheral = PeripheralId::SPI0;
    static constexpr SignalType type = SignalType::MISO;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA24, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC16, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief SPI0 MOSI signal
 * Compatible pins: PA23, PC17
 */
struct SPI0MOSISignal {
    static constexpr PeripheralId peripheral = PeripheralId::SPI0;
    static constexpr SignalType type = SignalType::MOSI;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA23, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC17, AlternateFunction::PERIPH_C}
    };
};

// TC Signals
// ------------------------------------------------------------------------------

/**
 * @brief TC0 CLK signal
 * Compatible pins: PA13, PA28, PA29, PA4, PD27, PD30
 */
struct TC0CLKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC0;
    static constexpr SignalType type = SignalType::CLK;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA13, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA28, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA29, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA4, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD27, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD30, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC0 TIOA signal
 * Compatible pins: PA0, PA14, PA16, PA26, PA31, PD25, PD28, PD31
 */
struct TC0TIOASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA0, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA14, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA16, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA26, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA31, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD25, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD28, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD31, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC0 TIOB signal
 * Compatible pins: PA1, PA15, PA17, PA27, PD26, PD29
 */
struct TC0TIOBSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA1, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA15, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA17, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA27, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD26, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD29, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC1 CLK signal
 * Compatible pins: PC28, PC31, PC7
 */
struct TC1CLKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC1;
    static constexpr SignalType type = SignalType::CLK;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PC28, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC31, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC7, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC1 TIOA signal
 * Compatible pins: PC26, PC29, PC5
 */
struct TC1TIOASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PC26, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC29, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC5, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC1 TIOB signal
 * Compatible pins: PC27, PC30, PC6
 */
struct TC1TIOBSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PC27, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC30, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC6, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC2 CLK signal
 * Compatible pins: PD17, PD20, PD23
 */
struct TC2CLKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC2;
    static constexpr SignalType type = SignalType::CLK;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD17, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD20, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD23, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC2 TIOA signal
 * Compatible pins: PD15, PD18, PD21
 */
struct TC2TIOASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD15, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD18, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD21, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief TC2 TIOB signal
 * Compatible pins: PD16, PD19, PD22
 */
struct TC2TIOBSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TC2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD16, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD19, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD22, AlternateFunction::PERIPH_C}
    };
};

// TWI Signals
// ------------------------------------------------------------------------------

/**
 * @brief TWI0 SCL signal
 * Compatible pins: PA4, PB13
 */
struct TWI0SCLSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TWI0;
    static constexpr SignalType type = SignalType::SCL;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA4, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB13, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief TWI0 SDA signal
 * Compatible pins: PA3, PB12
 */
struct TWI0SDASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TWI0;
    static constexpr SignalType type = SignalType::SDA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA3, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB12, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief TWI1 SCL signal
 * Compatible pins: PB3, PB5
 */
struct TWI1SCLSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TWI1;
    static constexpr SignalType type = SignalType::SCL;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB3, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB5, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief TWI1 SDA signal
 * Compatible pins: PB2, PB4
 */
struct TWI1SDASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TWI1;
    static constexpr SignalType type = SignalType::SDA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB2, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB4, AlternateFunction::PERIPH_A}
    };
};

// USART Signals
// ------------------------------------------------------------------------------

/**
 * @brief USART0 TX signal
 * Compatible pins: PA10, PA6
 */
struct USART0TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART0;
    static constexpr SignalType type = SignalType::TX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA10, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA6, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1 RX signal
 * Compatible pins: PA21, PB7
 */
struct USART1RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::RX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA21, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB7, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1 TX signal
 * Compatible pins: PA22, PB6
 */
struct USART1TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::TX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA22, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB6, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART2 RX signal
 * Compatible pins: PB9, PD20, PD25
 */
struct USART2RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART2;
    static constexpr SignalType type = SignalType::RX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB9, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD20, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD25, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART2 TX signal
 * Compatible pins: PB8, PD21, PD26
 */
struct USART2TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART2;
    static constexpr SignalType type = SignalType::TX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB8, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD21, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD26, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART3 RX signal
 * Compatible pins: PB11, PD30
 */
struct USART3RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART3;
    static constexpr SignalType type = SignalType::RX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB11, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD30, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART3 TX signal
 * Compatible pins: PB10, PD29
 */
struct USART3TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART3;
    static constexpr SignalType type = SignalType::TX;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB10, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD29, AlternateFunction::PERIPH_A}
    };
};


}  // namespace alloy::hal::atmel::same70
