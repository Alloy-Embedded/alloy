/**
 * @file spi.hpp
 * @brief STM32F7 Platform-Specific SPI Type Aliases
 *
 * Provides convenient type aliases for STM32F7 SPI peripherals using
 * the three-tier API system (Simple, Fluent, Expert).
 *
 * Clock Configuration (STM32F722):
 * - APB2 (SPI1, SPI4, SPI5, SPI6): 108 MHz
 * - APB1 (SPI2, SPI3): 54 MHz
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/spi_simple.hpp, spi_fluent.hpp, spi_expert.hpp
 * 2. Integration Layer     -> This file (platform/stm32f7/spi.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32f7;
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

namespace ucore::hal::stm32f7 {

using namespace ucore::hal::signals;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief SPI1 Simple API
 *
 * Common pins on Nucleo F722ZE:
 * - MOSI: PA7 (Arduino D11) or PB5
 * - MISO: PA6 (Arduino D12) or PB4
 * - SCK:  PA5 (Arduino D13) or PB3
 * - NSS:  PA4 or PA15
 *
 * Clock: APB2 @ 108 MHz
 * Max Speed: 54 Mbit/s (APB2 / 2)
 */
using Spi1 = Spi<PeripheralId::SPI1>;

/**
 * @brief SPI2 Simple API
 *
 * Common pins:
 * - MOSI: PB15 or PC3
 * - MISO: PB14 or PC2
 * - SCK:  PB13 or PD3
 * - NSS:  PB12 or PB9
 *
 * Clock: APB1 @ 54 MHz
 * Max Speed: 27 Mbit/s (APB1 / 2)
 */
using Spi2 = Spi<PeripheralId::SPI2>;

/**
 * @brief SPI3 Simple API
 *
 * Common pins:
 * - MOSI: PB5 or PC12
 * - MISO: PB4 or PC11
 * - SCK:  PB3 or PC10
 * - NSS:  PA15 or PA4
 *
 * Clock: APB1 @ 54 MHz
 * Max Speed: 27 Mbit/s (APB1 / 2)
 */
using Spi3 = Spi<PeripheralId::SPI3>;

/**
 * @brief SPI4 Simple API
 *
 * Common pins:
 * - MOSI: PE14 or PE6
 * - MISO: PE13 or PE5
 * - SCK:  PE12 or PE2
 * - NSS:  PE11 or PE4
 *
 * Clock: APB2 @ 108 MHz
 * Max Speed: 54 Mbit/s (APB2 / 2)
 */
using Spi4 = Spi<PeripheralId::SPI4>;

/**
 * @brief SPI5 Simple API
 *
 * Common pins:
 * - MOSI: PF9 or PF11
 * - MISO: PF8 or PH7
 * - SCK:  PF7 or PH6
 * - NSS:  PF6 or PH5
 *
 * Clock: APB2 @ 108 MHz
 * Max Speed: 54 Mbit/s (APB2 / 2)
 */
using Spi5 = Spi<PeripheralId::SPI5>;

/**
 * @brief SPI6 Simple API
 *
 * Common pins:
 * - MOSI: PG14
 * - MISO: PG12
 * - SCK:  PG13
 * - NSS:  PG8
 *
 * Clock: APB2 @ 108 MHz
 * Max Speed: 54 Mbit/s (APB2 / 2)
 */
using Spi6 = Spi<PeripheralId::SPI6>;

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
using Spi4Builder = SpiBuilder<PeripheralId::SPI4>;
using Spi5Builder = SpiBuilder<PeripheralId::SPI5>;
using Spi6Builder = SpiBuilder<PeripheralId::SPI6>;

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
using Spi4Expert = SpiExpert<PeripheralId::SPI4>;
using Spi5Expert = SpiExpert<PeripheralId::SPI5>;
using Spi6Expert = SpiExpert<PeripheralId::SPI6>;

}  // namespace ucore::hal::stm32f7

/**
 * @example STM32F7 SPI Example
 * @code
 * #include "hal/platform/stm32f7/spi.hpp"
 *
 * using namespace ucore::hal::stm32f7;
 *
 * // Define pin mappings (example for Nucleo F722ZE)
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
