/**
 * @file signals.hpp
 * @brief Signal Routing Tables for ATSAME70N21B
 *
 * Auto-generated signal routing tables for peripheral signal validation.
 * Part of Phase 2: Signal Metadata Generation.
 *
 * MCU: ATSAME70N21B
 * Package: N-pin (100 pins)
 * Available Ports: PA, PB, PC, PD
 *
 * Generated from: same70_pin_functions.py
 * Generator: signals_generator.py
 * Generated: 2025-11-11 21:54:48
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

// PWM Signals
// ------------------------------------------------------------------------------


/**
 * @brief PWM0_HIGH signal
 * Compatible pins: PA0, PA1, PA11, PA12, PA2, PA30, PA7, PB0, PB1, PB4, PC0, PC1, PC12, PC13, PC14, PC15, PC2, PC3, PD0, PD1, PD2, PD24, PD3
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
        PinDef{PinId::PD3, AlternateFunction::PERIPH_B}
    };
};

/**
 * @brief PWM0_LOW signal
 * Compatible pins: PA8, PB5, PC10, PC11, PC8, PC9, PD4, PD5, PD6, PD7
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
        PinDef{PinId::PD7, AlternateFunction::PERIPH_B}
    };
};

/**
 * @brief PWM0_DATA signal
 * Compatible pins: PD10, PD11, PD12, PD13, PD14, PD8, PD9
 */
struct PWM0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::PWM0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD10, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD11, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD12, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD13, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD14, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD8, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD9, AlternateFunction::PERIPH_B}
    };
};
// TC Signals
// ------------------------------------------------------------------------------


/**
 * @brief Timer0_TIOA signal
 * Compatible pins: PA0, PA14, PA16, PA26, PA31, PC5, PD15, PD25, PD28, PD31
 */
struct TIMER0TIOASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA0, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA14, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA16, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA26, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA31, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC5, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD15, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD25, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD28, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD31, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief Timer0_TIOB signal
 * Compatible pins: PA1, PA15, PA17, PA27, PC6, PD16, PD26, PD29
 */
struct TIMER0TIOBSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA1, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA15, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA17, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA27, AlternateFunction::PERIPH_B},
        PinDef{PinId::PC6, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD16, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD26, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD29, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief Timer0_CLOCK signal
 * Compatible pins: PA13, PA28, PA29, PA4, PD27, PD30
 */
struct TIMER0CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER0;
    static constexpr SignalType type = SignalType::DATA;
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
 * @brief Timer1_TIOA signal
 * Compatible pins: PC26, PC29, PD18
 */
struct TIMER1TIOASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PC26, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC29, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD18, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief Timer1_TIOB signal
 * Compatible pins: PC27, PC30, PD19
 */
struct TIMER1TIOBSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PC27, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC30, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD19, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief Timer1_CLOCK signal
 * Compatible pins: PC28, PC31, PC7
 */
struct TIMER1CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PC28, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC31, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC7, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief Timer2_CLOCK signal
 * Compatible pins: PD17, PD20, PD23
 */
struct TIMER2CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD17, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD20, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD23, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief Timer2_TIOA signal
 * Compatible pins: PD21
 */
struct TIMER2TIOASignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD21, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief Timer2_TIOB signal
 * Compatible pins: PD22
 */
struct TIMER2TIOBSignal {
    static constexpr PeripheralId peripheral = PeripheralId::TIMER2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD22, AlternateFunction::PERIPH_C}
    };
};
// USART Signals
// ------------------------------------------------------------------------------


/**
 * @brief USART0_TX signal
 * Compatible pins: PA10, PA6
 */
struct USART0TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA10, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA6, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1_RX signal
 * Compatible pins: PA21, PB7
 */
struct USART1RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA21, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB7, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1_TX signal
 * Compatible pins: PA22, PB6
 */
struct USART1TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA22, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB6, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1_CLOCK signal
 * Compatible pins: PA23
 */
struct USART1CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA23, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1_RTS signal
 * Compatible pins: PA24
 */
struct USART1RTSSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA24, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1_CTS signal
 * Compatible pins: PA25
 */
struct USART1CTSSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA25, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART1_DATA signal
 * Compatible pins: PA26, PA27, PA28, PA29
 */
struct USART1DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA26, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA27, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA28, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA29, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART0_RX signal
 * Compatible pins: PA5, PA9
 */
struct USART0RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA5, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA9, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART0_RTS signal
 * Compatible pins: PA7
 */
struct USART0RTSSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA7, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief USART3_TX signal
 * Compatible pins: PB10, PD29
 */
struct USART3TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART3;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB10, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD29, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART3_RX signal
 * Compatible pins: PB11, PD30
 */
struct USART3RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART3;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB11, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD30, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART2_TX signal
 * Compatible pins: PB8, PD21, PD26
 */
struct USART2TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB8, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD21, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD26, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART2_RX signal
 * Compatible pins: PB9, PD20, PD25
 */
struct USART2RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB9, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD20, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD25, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART2_CLOCK signal
 * Compatible pins: PD22
 */
struct USART2CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD22, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART2_RTS signal
 * Compatible pins: PD23
 */
struct USART2RTSSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD23, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART2_CTS signal
 * Compatible pins: PD24
 */
struct USART2CTSSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART2;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD24, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief USART3_CLOCK signal
 * Compatible pins: PD31
 */
struct USART3CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::USART3;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD31, AlternateFunction::PERIPH_A}
    };
};
// ISI Signals
// ------------------------------------------------------------------------------


/**
 * @brief ISI0_DATA signal
 * Compatible pins: PA10, PA18, PA19, PA20, PA9, PB10, PB11, PB12, PB6, PB7, PB8, PB9
 */
struct ISI0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::ISI0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA10, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA18, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA19, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA20, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA9, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB10, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB11, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB12, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB6, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB7, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB8, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB9, AlternateFunction::PERIPH_B}
    };
};
// QSPI Signals
// ------------------------------------------------------------------------------


/**
 * @brief QSPI0_DATA signal
 * Compatible pins: PA11, PC20, PC21, PC22, PC23, PC24, PC25
 */
struct QSPI0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::QSPI0;
    static constexpr SignalType type = SignalType::CS;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA11, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC20, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC21, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC22, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC23, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC24, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC25, AlternateFunction::PERIPH_C}
    };
};
// HSMCI Signals
// ------------------------------------------------------------------------------


/**
 * @brief HSMCI0_DATA signal
 * Compatible pins: PA12, PA13, PA14, PA15, PA16, PA17, PA19, PA20
 */
struct HSMCI0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::HSMCI0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA12, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA13, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA14, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA15, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA16, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA17, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA19, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA20, AlternateFunction::PERIPH_A}
    };
};
// PMC Signals
// ------------------------------------------------------------------------------


/**
 * @brief PMC0_DATA signal
 * Compatible pins: PA18, PA21, PB13, PD27
 */
struct PMC0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::PMC0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA18, AlternateFunction::PERIPH_A},
        PinDef{PinId::PA21, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB13, AlternateFunction::PERIPH_B},
        PinDef{PinId::PD27, AlternateFunction::PERIPH_A}
    };
};
// DAC Signals
// ------------------------------------------------------------------------------


/**
 * @brief DAC_OUT signal
 * Compatible pins: PA2
 */
struct DACOUTSignal {
    static constexpr PeripheralId peripheral = PeripheralId::DAC;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA2, AlternateFunction::PERIPH_C}
    };
};
// SPI Signals
// ------------------------------------------------------------------------------


/**
 * @brief SPI0_CLOCK signal
 * Compatible pins: PA22, PC18
 */
struct SPI0CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::SPI0;
    static constexpr SignalType type = SignalType::CLOCK;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA22, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC18, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief SPI0_MOSI signal
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

/**
 * @brief SPI0_MISO signal
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
 * @brief SPI0_CS signal
 * Compatible pins: PA25, PA30, PA31, PC19, PC4, PD28
 */
struct SPI0CSSignal {
    static constexpr PeripheralId peripheral = PeripheralId::SPI0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA25, AlternateFunction::PERIPH_C},
        PinDef{PinId::PA30, AlternateFunction::PERIPH_B},
        PinDef{PinId::PA31, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC19, AlternateFunction::PERIPH_C},
        PinDef{PinId::PC4, AlternateFunction::PERIPH_C},
        PinDef{PinId::PD28, AlternateFunction::PERIPH_A}
    };
};
// TWI Signals
// ------------------------------------------------------------------------------


/**
 * @brief I2C0_DATA signal
 * Compatible pins: PA3, PB12
 */
struct I2C0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::I2C0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA3, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB12, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief I2C0_CLOCK signal
 * Compatible pins: PA4, PB13
 */
struct I2C0CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::I2C0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA4, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB13, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief I2C1_DATA signal
 * Compatible pins: PB2, PB4
 */
struct I2C1DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::I2C1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB2, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB4, AlternateFunction::PERIPH_A}
    };
};

/**
 * @brief I2C1_CLOCK signal
 * Compatible pins: PB3, PB5
 */
struct I2C1CLOCKSignal {
    static constexpr PeripheralId peripheral = PeripheralId::I2C1;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB3, AlternateFunction::PERIPH_B},
        PinDef{PinId::PB5, AlternateFunction::PERIPH_A}
    };
};
// LCD Signals
// ------------------------------------------------------------------------------


/**
 * @brief LCD0_DATA signal
 * Compatible pins: PA3
 */
struct LCD0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::LCD0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA3, AlternateFunction::PERIPH_C}
    };
};
// UART Signals
// ------------------------------------------------------------------------------


/**
 * @brief UART0_RX signal
 * Compatible pins: PA5
 */
struct UART0RXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::UART0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA5, AlternateFunction::PERIPH_C}
    };
};

/**
 * @brief UART0_TX signal
 * Compatible pins: PA6
 */
struct UART0TXSignal {
    static constexpr PeripheralId peripheral = PeripheralId::UART0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA6, AlternateFunction::PERIPH_C}
    };
};
// ADC Signals
// ------------------------------------------------------------------------------


/**
 * @brief ADC0_IN signal
 * Compatible pins: PA8, PB0, PB1
 */
struct ADC0INSignal {
    static constexpr PeripheralId peripheral = PeripheralId::ADC0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PA8, AlternateFunction::PERIPH_C},
        PinDef{PinId::PB0, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB1, AlternateFunction::PERIPH_A}
    };
};
// CAN Signals
// ------------------------------------------------------------------------------


/**
 * @brief CAN0_DATA signal
 * Compatible pins: PB2, PB3
 */
struct CAN0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::CAN0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PB2, AlternateFunction::PERIPH_A},
        PinDef{PinId::PB3, AlternateFunction::PERIPH_A}
    };
};
// EBI Signals
// ------------------------------------------------------------------------------


/**
 * @brief EBI0_DATA signal
 * Compatible pins: PC0, PC1, PC10, PC11, PC12, PC13, PC14, PC15, PC16, PC17, PC18, PC19, PC2, PC20, PC21, PC22, PC23, PC24, PC25, PC26, PC27, PC28, PC29, PC3, PC30, PC31, PC4, PC5, PC6, PC7, PC8, PC9
 */
struct EBI0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::EBI0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PC0, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC1, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC10, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC11, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC12, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC13, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC14, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC15, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC16, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC17, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC18, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC19, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC2, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC20, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC21, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC22, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC23, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC24, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC25, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC26, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC27, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC28, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC29, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC3, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC30, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC31, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC4, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC5, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC6, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC7, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC8, AlternateFunction::PERIPH_A},
        PinDef{PinId::PC9, AlternateFunction::PERIPH_A}
    };
};
// GMAC Signals
// ------------------------------------------------------------------------------


/**
 * @brief GMAC0_DATA signal
 * Compatible pins: PD0, PD1, PD10, PD11, PD12, PD13, PD14, PD15, PD16, PD17, PD18, PD19, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9
 */
struct GMAC0DATASignal {
    static constexpr PeripheralId peripheral = PeripheralId::GMAC0;
    static constexpr SignalType type = SignalType::DATA;
    static constexpr std::array compatible_pins = {
        PinDef{PinId::PD0, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD1, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD10, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD11, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD12, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD13, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD14, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD15, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD16, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD17, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD18, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD19, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD2, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD3, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD4, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD5, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD6, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD7, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD8, AlternateFunction::PERIPH_A},
        PinDef{PinId::PD9, AlternateFunction::PERIPH_A}
    };
};

} // namespace alloy::hal::atmel::same70