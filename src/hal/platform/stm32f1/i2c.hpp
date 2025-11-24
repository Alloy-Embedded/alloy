/**
 * @file i2c.hpp
 * @brief STM32F1 Platform-Specific I2C Type Aliases
 *
 * Provides convenient type aliases for STM32F1 I2C peripherals using
 * the three-tier API system (Simple, Fluent, Expert).
 *
 * Clock Configuration (STM32F103):
 * - APB1 (I2C1, I2C2): 36 MHz
 *
 * STM32F1 I2C Features:
 * - Legacy register architecture (CR1/CR2/SR1/SR2/DR/CCR/TRISE)
 * - Standard-mode (100 kHz) and Fast-mode (400 kHz)
 * - 7-bit and 10-bit addressing
 * - Multi-master capability
 * - SMBus and PMBus support
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/i2c_simple.hpp, i2c_fluent.hpp, i2c_expert.hpp
 * 2. Integration Layer     -> This file (platform/stm32f1/i2c.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32f1;
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
 *     PinId::PB7, PinId::PB6);
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

namespace ucore::hal::stm32f1 {

using namespace ucore::hal::signals;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief I2C1 Simple API
 *
 * Common pins on Blue Pill (STM32F103C8):
 * - SDA: PB7 (I2C1_SDA)
 * - SCL: PB6 (I2C1_SCL)
 *
 * Alternate pins (remap):
 * - SDA: PB9 (remap)
 * - SCL: PB8 (remap)
 *
 * Clock: APB1 @ 36 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast)
 */
using I2c1 = I2c<PeripheralId::I2C1>;

/**
 * @brief I2C2 Simple API
 *
 * Common pins:
 * - SDA: PB11 (I2C2_SDA)
 * - SCL: PB10 (I2C2_SCL)
 *
 * Clock: APB1 @ 36 MHz
 * Speeds: 100 kHz (Standard), 400 kHz (Fast)
 */
using I2c2 = I2c<PeripheralId::I2C2>;

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

// ============================================================================
// Level 3: Expert API Type Aliases
// ============================================================================

/**
 * @brief I2C1 Expert Configuration API
 *
 * Example:
 * @code
 * constexpr auto config = I2c1Expert::standard_100khz(
 *     PinId::PB7,  // SDA
 *     PinId::PB6   // SCL
 * );
 *
 * expert::configure(config);
 * expert::write<I2c1Expert>(0x50, data, length);
 * @endcode
 */
using I2c1Expert = I2cExpert<PeripheralId::I2C1>;

using I2c2Expert = I2cExpert<PeripheralId::I2C2>;

}  // namespace ucore::hal::stm32f1

/**
 * @example STM32F1 I2C Example
 * @code
 * #include "hal/platform/stm32f1/i2c.hpp"
 *
 * using namespace ucore::hal::stm32f1;
 *
 * // Define pin mappings (example for Blue Pill)
 * using I2cSda = Pin<PeripheralId::I2C1, PinId::PB7>;
 * using I2cScl = Pin<PeripheralId::I2C1, PinId::PB6>;
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
