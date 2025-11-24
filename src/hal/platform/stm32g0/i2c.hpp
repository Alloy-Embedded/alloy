/**
 * @file i2c.hpp
 * @brief STM32G0 Platform-Specific I2C Type Aliases
 *
 * Provides convenient type aliases for STM32G0 I2C peripherals using
 * the three-tier API system (Simple, Fluent, Expert).
 *
 * Clock Configuration (STM32G071/G0B1):
 * - APB1 (I2C1, I2C2, I2C3): 64 MHz
 *
 * STM32G0 I2C Features:
 * - Modern register architecture (TIMINGR/ISR/ICR) - same as STM32F7
 * - Fast-mode Plus (1 MHz)
 * - 7-bit and 10-bit addressing
 * - Clock stretching
 * - Wakeup from STOP mode
 * - SMBus 3.0 and PMBus support
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/i2c_simple.hpp, i2c_fluent.hpp, i2c_expert.hpp
 * 2. Integration Layer     -> This file (platform/stm32g0/i2c.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32g0;
 *
 * // Level 1: Simple API - One-liner setup
 * auto i2c = I2c1::quick_setup<SdaPin, SclPin>(I2cSpeed::Standard_100kHz);
 * i2c.initialize();
 * i2c.write(0x50, data, sizeof(data));
 *
 * // Level 2: Fluent API - Builder pattern
 * auto builder = I2c1Builder()
 *     .with_pins<SdaPin, SclPin>()
 *     .speed(I2cSpeed::Fast_400kHz)
 *     .initialize();
 *
 * // Level 3: Expert API - Full control with low-power
 * constexpr auto config = I2c1Expert::standard_100khz(
 *     PinId::PB9, PinId::PB8);
 * expert::configure(config);
 * expert::enable_wakeup();  // Wake from STOP mode
 * @endcode
 *
 * @note Part of Phase 3.4: I2C Implementation
 * @see docs/API_TIERS.md
 */

#pragma once

#include "hal/api/i2c_expert.hpp"
#include "hal/api/i2c_fluent.hpp"
#include "hal/api/i2c_simple.hpp"

#include "hal/core/signals.hpp"

namespace ucore::hal::stm32g0 {

using namespace ucore::hal::signals;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief I2C1 Simple API
 *
 * Common pins on Nucleo G071RB/G0B1RE:
 * - SDA: PB9 (Arduino D14) or PB7 or PA10
 * - SCL: PB8 (Arduino D15) or PB6 or PA9
 *
 * Clock: APB1 @ 64 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast), 1 MHz (Fast-mode Plus)
 * Features: Wakeup from STOP mode
 */
using I2c1 = I2c<PeripheralId::I2C1>;

/**
 * @brief I2C2 Simple API
 *
 * Common pins:
 * - SDA: PB11 or PB14 or PA12
 * - SCL: PB10 or PB13 or PA11
 *
 * Clock: APB1 @ 64 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast), 1 MHz (Fast-mode Plus)
 * Features: Wakeup from STOP mode
 */
using I2c2 = I2c<PeripheralId::I2C2>;

/**
 * @brief I2C3 Simple API (G0B1 only)
 *
 * Common pins:
 * - SDA: PC1 or PB4
 * - SCL: PC0 or PA8
 *
 * Clock: APB1 @ 64 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast), 1 MHz (Fast-mode Plus)
 * Features: Wakeup from STOP mode
 *
 * @note Only available on STM32G0B1 devices
 */
using I2c3 = I2c<PeripheralId::I2C3>;

// ============================================================================
// Level 2: Fluent API Type Aliases
// ============================================================================

/**
 * @brief I2C1 Fluent Builder API
 *
 * Example:
 * @code
 * auto i2c = I2c1Builder()
 *     .with_pins<SdaPin, SclPin>()
 *     .speed(I2cSpeed::Fast_400kHz)
 *     .addressing_mode(I2cAddressing::SevenBit)
 *     .enable_wakeup()  // Low-power feature
 *     .initialize();
 * @endcode
 */
using I2c1Builder = I2cBuilder<PeripheralId::I2C1>;

using I2c2Builder = I2cBuilder<PeripheralId::I2C2>;
using I2c3Builder = I2cBuilder<PeripheralId::I2C3>;

// ============================================================================
// Level 3: Expert API Type Aliases
// ============================================================================

/**
 * @brief I2C1 Expert Configuration API
 *
 * Example:
 * @code
 * constexpr auto config = I2c1Expert::standard_100khz(
 *     PinId::PB9,  // SDA
 *     PinId::PB8   // SCL
 * );
 *
 * expert::configure(config);
 * expert::enable_wakeup();  // STM32G0-specific low-power feature
 * expert::write<I2c1Expert>(0x50, data, length);
 * @endcode
 */
using I2c1Expert = I2cExpert<PeripheralId::I2C1>;

using I2c2Expert = I2cExpert<PeripheralId::I2C2>;
using I2c3Expert = I2cExpert<PeripheralId::I2C3>;

}  // namespace ucore::hal::stm32g0

/**
 * @example STM32G0 I2C Example
 * @code
 * #include "hal/platform/stm32g0/i2c.hpp"
 *
 * using namespace ucore::hal::stm32g0;
 *
 * // Define pin mappings (example for Nucleo G071RB)
 * using I2cSda = Pin<PeripheralId::I2C1, PinId::PB9>;  // Arduino D14
 * using I2cScl = Pin<PeripheralId::I2C1, PinId::PB8>;  // Arduino D15
 *
 * int main() {
 *     // Simple API - One-liner setup
 *     auto i2c = I2c1::quick_setup<I2cSda, I2cScl>(I2cSpeed::Standard_100kHz);
 *     i2c.initialize();
 *
 *     // Write to EEPROM at address 0x50
 *     uint8_t data[] = {0x00, 0x00, 0xAA, 0xBB};
 *     i2c.write(0x50, data, sizeof(data));
 *
 *     // Read from sensor
 *     uint8_t buffer[2];
 *     i2c.read(0x68, buffer, sizeof(buffer));
 *
 *     // Low-power operation (STM32G0-specific)
 *     i2c.enable_wakeup();
 *     enter_stop_mode();  // I2C can wake MCU on address match
 *
 *     // Cleanup
 *     i2c.deinitialize();
 * }
 * @endcode
 */
