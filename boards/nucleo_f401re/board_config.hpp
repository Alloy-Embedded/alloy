#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-F401RE
 *
 * Auto-generated from board.yaml
 * DO NOT EDIT MANUALLY - Run `ucore generate-board` to regenerate
 *
 * Board: Nucleo-F401RE
 * Vendor: STMicroelectronics
 * MCU: STM32F401RET6
 * Platform: stm32f4
 */

#include <cstdint>
#include "hal/vendors/st/stm32f4/gpio.hpp"
#include "hal/vendors/st/stm32f4/stm32f401/peripherals.hpp"

namespace nucleo_f401re {

using namespace ucore::hal::st::stm32f4;
using namespace ucore::generated::stm32f401;

// =============================================================================
// Board Information
// =============================================================================

struct BoardInfo {
    static constexpr const char* name = "Nucleo-F401RE";
    static constexpr const char* vendor = "STMicroelectronics";
    static constexpr const char* version = "1.0.0";
    static constexpr const char* mcu = "STM32F401RET6";
    static constexpr const char* architecture = "cortex-m4";
    static constexpr const char* description = "STM32 Nucleo-64 development board with STM32F401RE MCU";
    static constexpr const char* url = "https://www.st.com/en/evaluation-tools/nucleo-f401re.html";
};

// =============================================================================
// Clock Configuration
// =============================================================================

/**
 * @brief Clock configuration for Nucleo-F401RE
 *
 * Clock Source: PLL
 * System Clock: 84000000 Hz (84 MHz)
 * HSE: 8000000 Hz (8 MHz external crystal)
 *
 * PLL Configuration:
 *   Input:  8000000 Hz (HSE)
 *   PLL input: HSE / M = 8000000 Hz / 4 = 2000000 Hz
 *   VCO:    2000000 Hz × 168 = 336000000 Hz
 *   SYSCLK: VCO / P = 336000000 Hz / 4 = 84000000 Hz
 *   USB:    VCO / Q = 336000000 Hz / 7 = 48000000 Hz
 *
 * Bus Clocks:
 *   AHB:  84000000 Hz / 1 = 84000000 Hz
 *   APB1: 84000000 Hz / 2 = 42000000 Hz
 *   APB2: 84000000 Hz / 1 = 84000000 Hz
 */
struct ClockConfig {
    /// HSE crystal frequency
    static constexpr uint32_t hse_hz = 8000000;

    /// Target system clock frequency
    static constexpr uint32_t system_clock_hz = 84000000;

    /// PLL input divider (PLL / pll_m)
    static constexpr uint32_t pll_m = 4;

    /// PLL VCO multiplier (input × pll_n)
    static constexpr uint32_t pll_n = 168;

    /// PLL system clock divider (VCO / pll_p_div)
    static constexpr uint32_t pll_p_div = 4;

    /// PLL USB/SDMMC divider (VCO / pll_q)
    static constexpr uint32_t pll_q = 7;

    /// Flash latency (wait states) for 84 MHz
    static constexpr uint32_t flash_latency = 2;

    /// AHB prescaler (SYSCLK / ahb_prescaler)
    static constexpr uint32_t ahb_prescaler = 1;

    /// APB1 prescaler (AHB / apb1_prescaler)
    static constexpr uint32_t apb1_prescaler = 2;

    /// APB2 prescaler (AHB / apb2_prescaler)
    static constexpr uint32_t apb2_prescaler = 1;
};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// User LED LD2 (green) - Arduino D13 compatible
    using led_green = GpioPin<peripherals::GPIOA, 5>;

    /// led_green is active HIGH
    static constexpr bool led_green_active_high = true;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// User button B1 (blue) - Active LOW
    using button_user = GpioPin<peripherals::GPIOC, 13>;

    /// button_user is active LOW
    static constexpr bool button_user_active_high = false;

};

// =============================================================================
// UART Configuration
// =============================================================================

struct UartConfig {
    /// ST-Link Virtual COM Port (Arduino D1/D0)
    /// Instance: USART2, Baud: 115200
    using console_tx = GpioPin<peripherals::GPIOA, 2>;
    using console_rx = GpioPin<peripherals::GPIOA, 3>;

    static constexpr uint32_t console_baud_rate = 115200;
};

// =============================================================================
// SPI Configuration
// =============================================================================

struct SpiConfig {
    /// Arduino SPI header (D13/D11/D12)
    /// Instance: SPI1
    using arduino_spi_sck = GpioPin<peripherals::GPIOA, 5>;
    using arduino_spi_mosi = GpioPin<peripherals::GPIOA, 7>;
    using arduino_spi_miso = GpioPin<peripherals::GPIOA, 6>;
};

// =============================================================================
// I2C Configuration
// =============================================================================

struct I2cConfig {
    /// Arduino I2C header (D15/D14)
    /// Instance: I2C1, Speed: 100000 Hz
    using arduino_i2c_scl = GpioPin<peripherals::GPIOB, 8>;
    using arduino_i2c_sda = GpioPin<peripherals::GPIOB, 9>;

    static constexpr uint32_t arduino_i2c_speed_hz = 100000;
};

} // namespace nucleo_f401re
