#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-F722ZE
 *
 * Auto-generated from board.yaml
 * DO NOT EDIT MANUALLY - Run `ucore generate-board` to regenerate
 *
 * Board: Nucleo-F722ZE
 * Vendor: STMicroelectronics
 * MCU: STM32F722ZET6
 * Platform: stm32f7
 */

#include <cstdint>
#include "hal/vendors/st/stm32f7/gpio.hpp"
#include "hal/vendors/st/stm32f7/stm32f722/peripherals.hpp"

namespace nucleo_f722ze {

using namespace ucore::hal::st::stm32f7;
using namespace ucore::generated::stm32f722;

// =============================================================================
// Board Information
// =============================================================================

struct BoardInfo {
    static constexpr const char* name = "Nucleo-F722ZE";
    static constexpr const char* vendor = "STMicroelectronics";
    static constexpr const char* version = "1.0.0";
    static constexpr const char* mcu = "STM32F722ZET6";
    static constexpr const char* architecture = "cortex-m7";
    static constexpr const char* description = "STM32 Nucleo-144 development board with STM32F722ZE MCU";
    static constexpr const char* url = "https://www.st.com/en/evaluation-tools/nucleo-f722ze.html";
};

// =============================================================================
// Clock Configuration
// =============================================================================

/**
 * @brief Clock configuration for Nucleo-F722ZE
 *
 * Clock Source: PLL
 * System Clock: 180000000 Hz (180 MHz)
 * HSE: 8000000 Hz (8 MHz external crystal)
 *
 * PLL Configuration:
 *   Input:  8000000 Hz (HSE)
 *   PLL input: HSE / M = 8000000 Hz / 4 = 2000000 Hz
 *   VCO:    2000000 Hz × 180 = 360000000 Hz
 *   SYSCLK: VCO / P = 360000000 Hz / 2 = 180000000 Hz
 *   USB:    VCO / Q = 360000000 Hz / 8 = 45000000 Hz
 *
 * Bus Clocks:
 *   AHB:  180000000 Hz / 1 = 180000000 Hz
 *   APB1: 180000000 Hz / 4 = 45000000 Hz
 *   APB2: 180000000 Hz / 2 = 90000000 Hz
 */
struct ClockConfig {
    /// HSE crystal frequency
    static constexpr uint32_t hse_hz = 8000000;

    /// Target system clock frequency
    static constexpr uint32_t system_clock_hz = 180000000;

    /// PLL input divider (PLL / pll_m)
    static constexpr uint32_t pll_m = 4;

    /// PLL VCO multiplier (input × pll_n)
    static constexpr uint32_t pll_n = 180;

    /// PLL system clock divider (VCO / pll_p_div)
    static constexpr uint32_t pll_p_div = 2;

    /// PLL USB/SDMMC divider (VCO / pll_q)
    static constexpr uint32_t pll_q = 8;

    /// Flash latency (wait states) for 180 MHz
    static constexpr uint32_t flash_latency = 5;

    /// AHB prescaler (SYSCLK / ahb_prescaler)
    static constexpr uint32_t ahb_prescaler = 1;

    /// APB1 prescaler (AHB / apb1_prescaler)
    static constexpr uint32_t apb1_prescaler = 4;

    /// APB2 prescaler (AHB / apb2_prescaler)
    static constexpr uint32_t apb2_prescaler = 2;
};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// User LED LD1
    using led_green = GpioPin<peripherals::GPIOB, 7>;

    /// led_green is active HIGH
    static constexpr bool led_green_active_high = true;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// User button B1 (active LOW)
    using button_user = GpioPin<peripherals::GPIOC, 13>;

    /// button_user is active LOW
    static constexpr bool button_user_active_high = false;

};

// =============================================================================
// UART Configuration
// =============================================================================

struct UartConfig {
    /// ST-Link Virtual COM Port
    /// Instance: USART2, Baud: 115200
    using console_tx = GpioPin<peripherals::GPIOA, 2>;
    using console_rx = GpioPin<peripherals::GPIOA, 3>;

    static constexpr uint32_t console_baud_rate = 115200;
};

// =============================================================================
// SPI Configuration
// =============================================================================


// =============================================================================
// I2C Configuration
// =============================================================================


} // namespace nucleo_f722ze
