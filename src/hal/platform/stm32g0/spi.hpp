/**
 * @file spi.hpp
 * @brief STM32G0 Platform-Specific SPI Type Aliases
 *
 * Provides convenient type aliases for STM32G0 SPI peripherals using
 * the three-tier API system (Simple, Fluent, Expert).
 *
 * Clock Configuration (STM32G071/G0B1):
 * - APB1 (SPI1, SPI2, SPI3): 64 MHz
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs (Simple, Fluent, Expert) accept PeripheralId as template parameter
 * - Type aliases provide convenient platform-specific access
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/spi_simple.hpp, spi_fluent.hpp, spi_expert.hpp
 * 2. Integration Layer     -> This file (platform/stm32g0/spi.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32g0;
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

namespace ucore::hal::stm32g0 {

using namespace ucore::hal::signals;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief SPI1 Simple API
 *
 * Common pins on Nucleo G071RB/G0B1RE:
 * - MOSI: PA7 (Arduino D11) or PB5
 * - MISO: PA6 (Arduino D12) or PB4
 * - SCK:  PA5 (Arduino D13) or PB3
 * - NSS:  PA4 (Arduino A2) or PA15
 *
 * Clock: APB1 @ 64 MHz
 * Max Speed: 32 Mbit/s (APB1 / 2)
 */
using Spi1 = Spi<PeripheralId::SPI1>;

/**
 * @brief SPI2 Simple API
 *
 * Common pins:
 * - MOSI: PB15 or PC3
 * - MISO: PB14 or PC2
 * - SCK:  PB13 or PD1
 * - NSS:  PB12 or PB9 or PD0
 *
 * Clock: APB1 @ 64 MHz
 * Max Speed: 32 Mbit/s (APB1 / 2)
 */
using Spi2 = Spi<PeripheralId::SPI2>;

/**
 * @brief SPI3 Simple API (G0B1 only)
 *
 * Common pins:
 * - MOSI: PB5 or PD4
 * - MISO: PB4 or PD3
 * - SCK:  PB3 or PC10
 * - NSS:  PA15 or PA4
 *
 * Clock: APB1 @ 64 MHz
 * Max Speed: 32 Mbit/s (APB1 / 2)
 *
 * @note Only available on STM32G0B1 devices
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
 *     .clock_speed(2000000)  // 2 MHz
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

}  // namespace ucore::hal::stm32g0

/**
 * @example STM32G0 SPI Example
 * @code
 * #include "hal/platform/stm32g0/spi.hpp"
 *
 * using namespace ucore::hal::stm32g0;
 *
 * // Define pin mappings (example for Nucleo G071RB)
 * using SpiMosi = Pin<PeripheralId::SPI1, PinId::PA7>;  // Arduino D11
 * using SpiMiso = Pin<PeripheralId::SPI1, PinId::PA6>;  // Arduino D12
 * using SpiSck  = Pin<PeripheralId::SPI1, PinId::PA5>;  // Arduino D13
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
