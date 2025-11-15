/**
 * @file peripherals.hpp
 * @brief STM32F722 Peripheral Base Addresses
 *
 * This file defines the base addresses for all peripherals on the STM32F722ZET6.
 * These addresses are used by platform-layer implementations to access hardware.
 *
 * MCU: STM32F722ZET6
 * - ARM Cortex-M7 @ 216 MHz
 * - 512 KB Flash
 * - 256 KB RAM
 * - FPU (single and double precision)
 * - ART Accelerator
 *
 * Reference: STM32F722xx datasheet (DS11853 Rev 8)
 *
 * @note Part of Alloy HAL Vendor Layer
 */

#pragma once

#include <cstdint>

namespace alloy::generated::stm32f722 {

/**
 * @brief STM32F722 Peripheral Base Addresses
 *
 * Memory map for STM32F722:
 * - Flash:     0x0800'0000 - 0x081F'FFFF (512 KB)
 * - SRAM1:     0x2000'0000 - 0x2001'FFFF (128 KB)
 * - SRAM2:     0x2002'0000 - 0x2003'FFFF (128 KB)
 * - Periph:    0x4000'0000 - 0x5FFF'FFFF
 */
namespace peripherals {

// ===========================================================================
// AHB1 Peripherals (High-speed bus)
// ===========================================================================

constexpr uintptr_t GPIOA   = 0x40020000;  ///< GPIO Port A
constexpr uintptr_t GPIOB   = 0x40020400;  ///< GPIO Port B
constexpr uintptr_t GPIOC   = 0x40020800;  ///< GPIO Port C
constexpr uintptr_t GPIOD   = 0x40020C00;  ///< GPIO Port D
constexpr uintptr_t GPIOE   = 0x40021000;  ///< GPIO Port E
constexpr uintptr_t GPIOF   = 0x40021400;  ///< GPIO Port F
constexpr uintptr_t GPIOG   = 0x40021800;  ///< GPIO Port G
constexpr uintptr_t GPIOH   = 0x40021C00;  ///< GPIO Port H
constexpr uintptr_t GPIOI   = 0x40022000;  ///< GPIO Port I

constexpr uintptr_t CRC     = 0x40023000;  ///< CRC Calculation Unit
constexpr uintptr_t RCC     = 0x40023800;  ///< Reset and Clock Control
constexpr uintptr_t FLASH   = 0x40023C00;  ///< Flash Interface
constexpr uintptr_t DMA1    = 0x40026000;  ///< DMA Controller 1
constexpr uintptr_t DMA2    = 0x40026400;  ///< DMA Controller 2

// ===========================================================================
// APB1 Peripherals (Low-speed bus, max 54 MHz)
// ===========================================================================

constexpr uintptr_t TIM2    = 0x40000000;  ///< Timer 2
constexpr uintptr_t TIM3    = 0x40000400;  ///< Timer 3
constexpr uintptr_t TIM4    = 0x40000800;  ///< Timer 4
constexpr uintptr_t TIM5    = 0x40000C00;  ///< Timer 5
constexpr uintptr_t TIM6    = 0x40001000;  ///< Timer 6 (Basic)
constexpr uintptr_t TIM7    = 0x40001400;  ///< Timer 7 (Basic)
constexpr uintptr_t TIM12   = 0x40001800;  ///< Timer 12
constexpr uintptr_t TIM13   = 0x40001C00;  ///< Timer 13
constexpr uintptr_t TIM14   = 0x40002000;  ///< Timer 14

constexpr uintptr_t RTC     = 0x40002800;  ///< Real-Time Clock
constexpr uintptr_t WWDG    = 0x40002C00;  ///< Window Watchdog
constexpr uintptr_t IWDG    = 0x40003000;  ///< Independent Watchdog

constexpr uintptr_t SPI2    = 0x40003800;  ///< SPI 2
constexpr uintptr_t SPI3    = 0x40003C00;  ///< SPI 3

constexpr uintptr_t USART2  = 0x40004400;  ///< USART 2
constexpr uintptr_t USART3  = 0x40004800;  ///< USART 3
constexpr uintptr_t UART4   = 0x40004C00;  ///< UART 4
constexpr uintptr_t UART5   = 0x40005000;  ///< UART 5

constexpr uintptr_t I2C1    = 0x40005400;  ///< I2C 1
constexpr uintptr_t I2C2    = 0x40005800;  ///< I2C 2
constexpr uintptr_t I2C3    = 0x40005C00;  ///< I2C 3

constexpr uintptr_t CAN1    = 0x40006400;  ///< CAN 1

constexpr uintptr_t PWR     = 0x40007000;  ///< Power Control
constexpr uintptr_t DAC     = 0x40007400;  ///< Digital-to-Analog Converter

// ===========================================================================
// APB2 Peripherals (High-speed bus, max 108 MHz)
// ===========================================================================

constexpr uintptr_t TIM1    = 0x40010000;  ///< Timer 1 (Advanced)
constexpr uintptr_t TIM8    = 0x40010400;  ///< Timer 8 (Advanced)

constexpr uintptr_t USART1  = 0x40011000;  ///< USART 1
constexpr uintptr_t USART6  = 0x40011400;  ///< USART 6

constexpr uintptr_t ADC1    = 0x40012000;  ///< ADC 1
constexpr uintptr_t ADC2    = 0x40012100;  ///< ADC 2
constexpr uintptr_t ADC3    = 0x40012200;  ///< ADC 3
constexpr uintptr_t ADC_COMMON = 0x40012300; ///< ADC Common Registers

constexpr uintptr_t SDMMC1  = 0x40012C00;  ///< SDMMC 1

constexpr uintptr_t SPI1    = 0x40013000;  ///< SPI 1
constexpr uintptr_t SPI4    = 0x40013400;  ///< SPI 4

constexpr uintptr_t SYSCFG  = 0x40013800;  ///< System Configuration
constexpr uintptr_t EXTI    = 0x40013C00;  ///< External Interrupt

constexpr uintptr_t TIM9    = 0x40014000;  ///< Timer 9
constexpr uintptr_t TIM10   = 0x40014400;  ///< Timer 10
constexpr uintptr_t TIM11   = 0x40014800;  ///< Timer 11

constexpr uintptr_t SAI1    = 0x40015800;  ///< Serial Audio Interface 1
constexpr uintptr_t SAI2    = 0x40015C00;  ///< Serial Audio Interface 2

// ===========================================================================
// AHB2 Peripherals
// ===========================================================================

constexpr uintptr_t USB_OTG_FS = 0x50000000;  ///< USB OTG Full Speed

constexpr uintptr_t RNG     = 0x50060800;  ///< Random Number Generator

// ===========================================================================
// Cortex-M7 System Peripherals
// ===========================================================================

constexpr uintptr_t SCS_BASE    = 0xE000E000;  ///< System Control Space
constexpr uintptr_t SysTick     = 0xE000E010;  ///< SysTick Timer
constexpr uintptr_t NVIC        = 0xE000E100;  ///< Nested Vector Interrupt Controller
constexpr uintptr_t SCB         = 0xE000ED00;  ///< System Control Block
constexpr uintptr_t MPU         = 0xE000ED90;  ///< Memory Protection Unit
constexpr uintptr_t FPU         = 0xE000EF30;  ///< Floating Point Unit
constexpr uintptr_t CACHE       = 0xE000EF50;  ///< Cache Control

} // namespace peripherals

} // namespace alloy::generated::stm32f722
