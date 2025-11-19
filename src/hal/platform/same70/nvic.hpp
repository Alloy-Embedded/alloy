#pragma once

/**
 * @file nvic.hpp
 * @brief SAME70 NVIC Platform Integration
 */

#include "hal/vendors/atmel/same70/nvic_hardware_policy.hpp"

namespace alloy::hal::same70 {

// NVIC instance (ARM Cortex-M7)
using Nvic = NvicHardware;

// SAME70-specific IRQ numbers
namespace irq {
    constexpr uint8_t SUPC    = 0;   ///< Supply Controller
    constexpr uint8_t RSTC    = 1;   ///< Reset Controller
    constexpr uint8_t RTC     = 2;   ///< Real-time Clock
    constexpr uint8_t RTT     = 3;   ///< Real-time Timer
    constexpr uint8_t WDT     = 4;   ///< Watchdog Timer
    constexpr uint8_t PMC     = 5;   ///< Power Management Controller
    constexpr uint8_t EFC     = 6;   ///< Enhanced Embedded Flash Controller
    constexpr uint8_t UART0   = 7;   ///< UART 0
    constexpr uint8_t UART1   = 8;   ///< UART 1
    constexpr uint8_t PIOA    = 10;  ///< Parallel I/O Controller A
    constexpr uint8_t PIOB    = 11;  ///< Parallel I/O Controller B
    constexpr uint8_t PIOC    = 12;  ///< Parallel I/O Controller C
    constexpr uint8_t USART0  = 13;  ///< USART 0
    constexpr uint8_t USART1  = 14;  ///< USART 1
    constexpr uint8_t USART2  = 15;  ///< USART 2
    constexpr uint8_t PIOD    = 16;  ///< Parallel I/O Controller D
    constexpr uint8_t PIOE    = 17;  ///< Parallel I/O Controller E
    constexpr uint8_t HSMCI   = 18;  ///< High Speed Multimedia Card Interface
    constexpr uint8_t TWIHS0  = 19;  ///< Two-wire Interface 0 (I2C)
    constexpr uint8_t TWIHS1  = 20;  ///< Two-wire Interface 1 (I2C)
    constexpr uint8_t SPI0    = 21;  ///< Serial Peripheral Interface 0
    constexpr uint8_t SSC     = 22;  ///< Synchronous Serial Controller
    constexpr uint8_t TC0     = 23;  ///< Timer Counter 0
    constexpr uint8_t TC1     = 24;  ///< Timer Counter 1
    constexpr uint8_t TC2     = 25;  ///< Timer Counter 2
    constexpr uint8_t AFEC0   = 29;  ///< Analog Front-End Controller 0 (ADC)
    constexpr uint8_t DACC    = 30;  ///< Digital-to-Analog Converter
    constexpr uint8_t PWM0    = 31;  ///< Pulse Width Modulation Controller 0
    constexpr uint8_t ICM     = 32;  ///< Integrity Check Monitor
    constexpr uint8_t ACC     = 33;  ///< Analog Comparator Controller
    constexpr uint8_t USBHS   = 34;  ///< USB High Speed
    constexpr uint8_t MCAN0   = 35;  ///< CAN Controller 0
    constexpr uint8_t MCAN1   = 37;  ///< CAN Controller 1
    constexpr uint8_t GMAC    = 39;  ///< Gigabit Ethernet MAC
    constexpr uint8_t AFEC1   = 40;  ///< Analog Front-End Controller 1 (ADC)
    constexpr uint8_t TWIHS2  = 41;  ///< Two-wire Interface 2 (I2C)
    constexpr uint8_t SPI1    = 42;  ///< Serial Peripheral Interface 1
    constexpr uint8_t QSPI    = 43;  ///< Quad SPI
    constexpr uint8_t UART2   = 44;  ///< UART 2
    constexpr uint8_t UART3   = 45;  ///< UART 3
    constexpr uint8_t UART4   = 46;  ///< UART 4
    constexpr uint8_t TC9     = 50;  ///< Timer Counter 9
    constexpr uint8_t TC10    = 51;  ///< Timer Counter 10
    constexpr uint8_t TC11    = 52;  ///< Timer Counter 11
    constexpr uint8_t MLB     = 53;  ///< MediaLB
    constexpr uint8_t AES     = 56;  ///< Advanced Encryption Standard
    constexpr uint8_t TRNG    = 57;  ///< True Random Number Generator
    constexpr uint8_t XDMAC   = 58;  ///< DMA Controller
    constexpr uint8_t ISI     = 59;  ///< Image Sensor Interface
    constexpr uint8_t PWM1    = 60;  ///< Pulse Width Modulation Controller 1
    constexpr uint8_t RSWDT   = 63;  ///< Reinforced Safety Watchdog Timer
} // namespace irq

} // namespace alloy::hal::same70
