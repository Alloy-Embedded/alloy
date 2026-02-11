#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for SAME70 Xplained Ultra
 *
 * Auto-generated from board.yaml
 * DO NOT EDIT MANUALLY - Run `ucore generate-board` to regenerate
 *
 * Board: SAME70 Xplained Ultra
 * Vendor: Microchip/Atmel
 * MCU: ATSAME70Q21B
 * Platform: same70
 */

#include <cstdint>
#include "hal/vendors/arm/same70/gpio.hpp"
#include "hal/vendors/arm/same70/clock.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace same70_xplained_ultra {

using namespace ucore::hal::same70;
using namespace ucore::generated::atsame70q21b;

// =============================================================================
// Board Information
// =============================================================================

struct BoardInfo {
    static constexpr const char* name = "SAME70 Xplained Ultra";
    static constexpr const char* vendor = "Microchip/Atmel";
    static constexpr const char* version = "1.0.0";
    static constexpr const char* mcu = "ATSAME70Q21B";
    static constexpr const char* architecture = "cortex-m7";
    static constexpr const char* description = "SAME70 Xplained Ultra evaluation board with ATSAME70Q21B MCU";
    static constexpr const char* url = "https://www.microchip.com/developmenttools/ProductDetails/atsame70-xpld";
};

// =============================================================================
// Clock Configuration
// =============================================================================

/**
 * @brief Clock configuration for SAME70 Xplained Ultra
 *
 * Clock Source: PLL
 * System Clock: 300000000 Hz (300 MHz)
 * HSE: 12000000 Hz (12 MHz external crystal)
 *
 * PLL Configuration:
 *   Input:  12000000 Hz (HSE)
 *   PLL input: HSE / M = 12000000 Hz / 1 = 12000000 Hz
 *   VCO:    12000000 Hz × 25 = 300000000 Hz
 *   SYSCLK: VCO / P = 300000000 Hz / 2 = 300000000 Hz
 *
 * Bus Clocks:
 *   AHB:  300000000 Hz / 1 = 300000000 Hz
 *   APB1: 300000000 Hz / 1 = 300000000 Hz
 *   APB2: 300000000 Hz / 1 = 300000000 Hz
 */
struct ClockConfig {
    /// HSE crystal frequency
    static constexpr uint32_t hse_hz = 12000000;

    /// Target system clock frequency
    static constexpr uint32_t system_clock_hz = 300000000;

    /// PLL input divider (PLL / pll_m)
    static constexpr uint32_t pll_m = 1;

    /// PLL VCO multiplier (input × pll_n)
    static constexpr uint32_t pll_n = 25;

    /// PLL system clock divider (VCO / pll_p_div)
    static constexpr uint32_t pll_p_div = 2;


    /// Flash latency (wait states) for 300 MHz
    static constexpr uint32_t flash_latency = 6;

    /// AHB prescaler (SYSCLK / ahb_prescaler)
    static constexpr uint32_t ahb_prescaler = 1;

    /// APB1 prescaler (AHB / apb1_prescaler)
    static constexpr uint32_t apb1_prescaler = 1;

    /// APB2 prescaler (AHB / apb2_prescaler)
    static constexpr uint32_t apb2_prescaler = 1;
};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// LED0 (Green) - Active LOW
    using led_green = GpioPin<peripherals::PIOC, 8>;

    /// led_green is active LOW
    static constexpr bool led_green_active_high = false;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// SW0 (User button) - Active LOW
    using button0 = GpioPin<peripherals::PIOA, 11>;

    /// button0 is active LOW
    static constexpr bool button0_active_high = false;

    /// button0 pull resistor: up
    static constexpr bool button0_pull_up = true;
};

// =============================================================================
// UART Configuration
// =============================================================================

struct UartConfig {
    /// UART1 for console (EDBG virtual COM port)
    /// Instance: UART1, Baud: 115200
    using console_tx = GpioPin<peripherals::PIOA, 9>;
    using console_rx = GpioPin<peripherals::PIOA, 10>;

    static constexpr uint32_t console_baud_rate = 115200;
};

// =============================================================================
// SPI Configuration
// =============================================================================


// =============================================================================
// I2C Configuration
// =============================================================================


} // namespace same70_xplained_ultra
