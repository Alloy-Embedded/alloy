#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-G071RB board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-G071RB
 * development board (MB1360).
 */

#include <cstdint>

#include "hal/connect.hpp"

namespace nucleo_g071rb {

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// Green LED (LD4) on PA5 - Arduino D13 compatible pin
    using led_green = alloy::hal::pin<"PA5">;

    /// LED is active HIGH (turns on when pin is HIGH)
    static constexpr bool led_green_active_high = true;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// User button (B1) on PC13 - Active LOW (pressed = LOW)
    using button_user = alloy::hal::pin<"PC13">;

    /// Button is active LOW
    static constexpr bool button_active_high = false;
};

// =============================================================================
// Clock Configuration
// =============================================================================

struct ClockConfig {
    /// System clock frequency (using HSI 64 MHz oscillator)
    /// Note: STM32G071 has HSI16 that can be multiplied to 64MHz via PLL
    static constexpr uint32_t system_clock_hz = 64'000'000;

    /// APB bus frequency (same as system clock on Cortex-M0+)
    static constexpr uint32_t apb_clock_hz = system_clock_hz;

    /// PLL input divider (/1)
    static constexpr uint32_t pll_m = 0;

    /// PLL multiplier (x8)
    static constexpr uint32_t pll_n = 8;

    /// PLL output divider (/2)
    static constexpr uint32_t pll_r = 0;

    /// Flash latency for 64 MHz
    static constexpr uint32_t flash_latency = 2;
};

// =============================================================================
// UART Configuration (ST-Link Virtual COM Port)
// =============================================================================

struct UartConfig {
    /// USART2 TX pin - PA2 (ST-Link VCP TX)
    using usart2_tx = alloy::hal::pin<"PA2">;

    /// USART2 RX pin - PA3 (ST-Link VCP RX)
    using usart2_rx = alloy::hal::pin<"PA3">;

    using debug_connector =
        decltype(alloy::hal::connect<alloy::hal::peripheral<"USART2">, alloy::hal::tx<usart2_tx>,
                                     alloy::hal::rx<usart2_rx>>());

    /// Default baud rate for debug UART
    static constexpr uint32_t default_baud_rate = 115200;
    static constexpr uint32_t peripheral_clock_hz = ClockConfig::apb_clock_hz;
};

struct I2cConfig {
    using scl = alloy::hal::pin<"PB6">;
    using sda = alloy::hal::pin<"PB7">;

    using bus_connector =
        decltype(alloy::hal::connect<alloy::hal::peripheral<"I2C1">, alloy::hal::scl<scl>,
                                     alloy::hal::sda<sda>>());

    static constexpr uint32_t peripheral_clock_hz = ClockConfig::apb_clock_hz;
};

struct SpiConfig {
    using sck = alloy::hal::pin<"PA5">;
    using miso = alloy::hal::pin<"PA6">;
    using mosi = alloy::hal::pin<"PA7">;

    using bus_connector =
        decltype(alloy::hal::connect<alloy::hal::peripheral<"SPI1">, alloy::hal::sck<sck>,
                                     alloy::hal::miso<miso>, alloy::hal::mosi<mosi>>());

    static constexpr uint32_t peripheral_clock_hz = ClockConfig::apb_clock_hz;
};

}  // namespace nucleo_g071rb
