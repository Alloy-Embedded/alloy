/**
 * @file i2c.hpp
 * @brief STM32F7 Platform-Specific I2C Type Aliases
 *
 * Provides convenient type aliases for STM32F7 I2C peripherals using
 * the three-tier API system (Simple, Fluent, Expert).
 *
 * Clock Configuration (STM32F722):
 * - APB1 (I2C1, I2C2, I2C3, I2C4): 54 MHz
 *
 * STM32F7 I2C Features:
 * - Modern register architecture (TIMINGR/ISR/ICR)
 * - Fast-mode Plus (1 MHz)
 * - 7-bit and 10-bit addressing
 * - Clock stretching
 * - SMBus and PMBus support
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/i2c_simple.hpp, i2c_fluent.hpp, i2c_expert.hpp
 * 2. Integration Layer     -> This file (platform/stm32f7/i2c.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32f7;
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
 * // Level 3: Expert API - Full control
 * constexpr auto config = I2c1Expert::standard_100khz(
 *     PinId::PB9, PinId::PB8);
 * expert::configure(config);
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

namespace ucore::hal::stm32f7 {

using namespace ucore::hal::signals;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief I2C1 Simple API
 *
 * Common pins on Nucleo F722ZE:
 * - SDA: PB9 (Arduino D14) or PB7
 * - SCL: PB8 (Arduino D15) or PB6
 *
 * Clock: APB1 @ 54 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast), 1 MHz (Fast-mode Plus)
 */
using I2c1 = I2c<PeripheralId::I2C1>;

/**
 * @brief I2C2 Simple API
 *
 * Common pins:
 * - SDA: PB11 or PB3 or PF0
 * - SCL: PB10 or PF1
 *
 * Clock: APB1 @ 54 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast), 1 MHz (Fast-mode Plus)
 */
using I2c2 = I2c<PeripheralId::I2C2>;

/**
 * @brief I2C3 Simple API
 *
 * Common pins:
 * - SDA: PC9 or PH8
 * - SCL: PA8 or PH7
 *
 * Clock: APB1 @ 54 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast), 1 MHz (Fast-mode Plus)
 */
using I2c3 = I2c<PeripheralId::I2C3>;

/**
 * @brief I2C4 Simple API
 *
 * Common pins:
 * - SDA: PD13 or PF15 or PH12
 * - SCL: PD12 or PF14 or PH11
 *
 * Clock: APB1 @ 54 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast), 1 MHz (Fast-mode Plus)
 */
using I2c4 = I2c<PeripheralId::I2C4>;

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
 *     .initialize();
 * @endcode
 */
using I2c1Builder = I2cBuilder<PeripheralId::I2C1>;

using I2c2Builder = I2cBuilder<PeripheralId::I2C2>;
using I2c3Builder = I2cBuilder<PeripheralId::I2C3>;
using I2c4Builder = I2cBuilder<PeripheralId::I2C4>;

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
 * expert::write<I2c1Expert>(0x50, data, length);
 * @endcode
 */
using I2c1Expert = I2cExpert<PeripheralId::I2C1>;

using I2c2Expert = I2cExpert<PeripheralId::I2C2>;
using I2c3Expert = I2cExpert<PeripheralId::I2C3>;
using I2c4Expert = I2cExpert<PeripheralId::I2C4>;

}  // namespace ucore::hal::stm32f7

/**
 * @example STM32F7 I2C Example
 * @code
 * #include "hal/platform/stm32f7/i2c.hpp"
 *
 * using namespace ucore::hal::stm32f7;
 *
 * // Define pin mappings (example for Nucleo F722ZE)
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
 *     // Cleanup
 *     i2c.deinitialize();
 * }
 * @endcode
 */
