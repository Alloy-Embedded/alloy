/**
 * @file spi.hpp
 * @brief STM32F1 Platform-Specific SPI Type Aliases
 *
 * Provides convenient type aliases for STM32F1 SPI peripherals using
 * the three-tier API system (Simple, Fluent, Expert).
 *
 * Clock Configuration (STM32F103):
 * - APB2 (SPI1): 72 MHz
 * - APB1 (SPI2, SPI3): 36 MHz
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/spi_simple.hpp, spi_fluent.hpp, spi_expert.hpp
 * 2. Integration Layer     -> This file (platform/stm32f1/spi.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32f1;
 *
 * // Level 1: Simple API - One-liner setup
 * auto spi = Spi1::quick_setup<MosiPin, MisoPin, SckPin>(1000000);  // 1 MHz
 * spi.initialize();
 * spi.transfer_byte(0x55);
 *
 * // Level 2: Fluent API - Builder pattern
 * auto builder = Spi1Builder()
 *     .with_pins<MosiPin, MisoPin, SckPin>()
 *     .clock_speed(1000000)
 *     .mode(SpiMode::Mode0)
 *     .initialize();
 *
 * // Level 3: Expert API - Full control
 * constexpr auto config = Spi1Expert::mode0_1mhz(
 *     PinId::PA7, PinId::PA6, PinId::PA5);
 * expert::configure(config);
 * @endcode
 *
 * @note Part of Phase 3.3: SPI Implementation
 * @see docs/API_TIERS.md
 */

#pragma once

#include "hal/api/spi_expert.hpp"
#include "hal/api/spi_fluent.hpp"
#include "hal/api/spi_simple.hpp"

#include "hal/core/signals.hpp"

namespace ucore::hal::stm32f1 {

using namespace ucore::hal::signals;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief SPI1 Simple API
 *
 * Common pins on Blue Pill (STM32F103C8):
 * - MOSI: PA7 (SPI1_MOSI)
 * - MISO: PA6 (SPI1_MISO)
 * - SCK:  PA5 (SPI1_SCK)
 * - NSS:  PA4 (SPI1_NSS)
 *
 * Alternate pins (remap):
 * - MOSI: PB5
 * - MISO: PB4
 * - SCK:  PB3
 * - NSS:  PA15
 *
 * Clock: APB2 @ 72 MHz
 * Max Speed: 18 Mbit/s (APB2 / 4, limited by GPIO speed)
 */
using Spi1 = Spi<PeripheralId::SPI1>;

/**
 * @brief SPI2 Simple API
 *
 * Common pins:
 * - MOSI: PB15 (SPI2_MOSI)
 * - MISO: PB14 (SPI2_MISO)
 * - SCK:  PB13 (SPI2_SCK)
 * - NSS:  PB12 (SPI2_NSS)
 *
 * Clock: APB1 @ 36 MHz
 * Max Speed: 9 Mbit/s (APB1 / 4, limited by GPIO speed)
 */
using Spi2 = Spi<PeripheralId::SPI2>;

/**
 * @brief SPI3 Simple API (High-Density Devices Only)
 *
 * Common pins:
 * - MOSI: PB5 (SPI3_MOSI)
 * - MISO: PB4 (SPI3_MISO)
 * - SCK:  PB3 (SPI3_SCK)
 * - NSS:  PA15 (SPI3_NSS)
 *
 * Alternate pins (remap):
 * - MOSI: PC12
 * - MISO: PC11
 * - SCK:  PC10
 * - NSS:  PA4
 *
 * Clock: APB1 @ 36 MHz
 * Max Speed: 9 Mbit/s (APB1 / 4, limited by GPIO speed)
 *
 * @note Only available on STM32F103xE (high-density) devices
 */
using Spi3 = Spi<PeripheralId::SPI3>;

// ============================================================================
// Level 2: Fluent API Type Aliases
// ============================================================================

/**
 * @brief SPI1 Fluent Builder API
 *
 * Example:
 * @code
 * auto spi = Spi1Builder()
 *     .with_pins<MosiPin, MisoPin, SckPin>()
 *     .clock_speed(1000000)  // 1 MHz
 *     .mode(SpiMode::Mode3)
 *     .bit_order(SpiBitOrder::LsbFirst)
 *     .initialize();
 * @endcode
 */
using Spi1Builder = SpiBuilder<PeripheralId::SPI1>;

using Spi2Builder = SpiBuilder<PeripheralId::SPI2>;
using Spi3Builder = SpiBuilder<PeripheralId::SPI3>;

// ============================================================================
// Level 3: Expert API Type Aliases
// ============================================================================

/**
 * @brief SPI1 Expert Configuration API
 *
 * Example:
 * @code
 * constexpr auto config = Spi1Expert::mode0_1mhz(
 *     PinId::PA7,  // MOSI
 *     PinId::PA6,  // MISO
 *     PinId::PA5   // SCK
 * );
 *
 * expert::configure(config);
 * expert::transfer_byte<decltype(config)>(0x55);
 * @endcode
 */
using Spi1Expert = SpiExpert<PeripheralId::SPI1>;

using Spi2Expert = SpiExpert<PeripheralId::SPI2>;
using Spi3Expert = SpiExpert<PeripheralId::SPI3>;

}  // namespace ucore::hal::stm32f1

/**
 * @example STM32F1 SPI Example
 * @code
 * #include "hal/platform/stm32f1/spi.hpp"
 *
 * using namespace ucore::hal::stm32f1;
 *
 * // Define pin mappings (example for Blue Pill)
 * using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;
 * using SpiMiso = Pin<PeripheralId::SPI1, PinId::PA6>;
 * using SpiSck  = Pin<PeripheralId::SPI1, PinId::PA5>;
 *
 * int main() {
 *     // Simple API - One-liner setup
 *     auto spi = Spi1::quick_setup<SpiMosi, SpiMiso, SpiSck>(1000000);
 *     spi.initialize();
 *
 *     // Transfer data
 *     uint8_t tx_data = 0x55;
 *     uint8_t rx_data = spi.transfer_byte(tx_data);
 *
 *     // Cleanup
 *     spi.deinitialize();
 * }
 * @endcode
 */
