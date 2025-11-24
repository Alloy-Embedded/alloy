/**
 * @file uart.hpp
 * @brief STM32F7 Platform-Specific UART Type Aliases
 *
 * Combines generic UART APIs with STM32F7 hardware policies to create
 * platform-specific UART instances. This is the integration point between
 * the generic API layer and the vendor-specific hardware policy.
 *
 * Clock Configuration (STM32F722):
 * - APB2 (USART1/6): 216 MHz
 * - APB1 (USART2/3, UART4/5/7/8): 108 MHz
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs (Simple, Fluent, Expert) accept HardwarePolicy as template parameter
 * - Hardware policy provides platform-specific implementation (STM32F7)
 * - Type aliases combine both for convenient usage
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/uart_simple.hpp, uart_fluent.hpp, uart_expert.hpp
 * 2. Hardware Policy Layer -> hal/vendors/st/stm32f7/usart_hardware_policy.hpp
 * 3. Integration Layer     -> This file (platform/stm32f7/uart.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32f7;
 *
 * // Level 1: Simple API - One-liner setup
 * auto uart = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
 * uart.initialize();
 *
 * // Level 2: Fluent API - Builder pattern
 * auto builder = Usart1Builder()
 *     .with_pins<TxPin, RxPin>()
 *     .baudrate(BaudRate{115200})
 *     .standard_8n1()
 *     .initialize();
 *
 * // Level 3: Expert API - Full control
 * constexpr auto config = Usart1ExpertConfig::standard_115200(
 *     PeripheralId::USART1, PinId::PA9, PinId::PA10);
 * expert::configure(config);
 * @endcode
 *
 * @note Part of Phase 3.2: UART Platform Completeness
 * @see docs/API_TIERS.md
 */

#pragma once

#include "hal/api/uart_expert.hpp"
#include "hal/api/uart_fluent.hpp"
#include "hal/api/uart_simple.hpp"
#include "hal/vendors/st/stm32f7/usart_hardware_policy.hpp"

#include "core/types.hpp"

namespace ucore::hal::stm32f7 {

using namespace ucore::core;
using namespace ucore::hal::signals;

// ============================================================================
// Hardware Policy Type Aliases (STM32F7 USART)
// ============================================================================

// USART1 (APB2 @ 216MHz on STM32F722)
using Usart1Hardware = Stm32f7UartHardwarePolicy<0x40011000, 216000000>;

// USART2 (APB1 @ 108MHz on STM32F722)
using Usart2Hardware = Stm32f7UartHardwarePolicy<0x40004400, 108000000>;

// USART3 (APB1 @ 108MHz)
using Usart3Hardware = Stm32f7UartHardwarePolicy<0x40004800, 108000000>;

// UART4 (APB1 @ 108MHz)
using Uart4Hardware = Stm32f7UartHardwarePolicy<0x40004C00, 108000000>;

// UART5 (APB1 @ 108MHz)
using Uart5Hardware = Stm32f7UartHardwarePolicy<0x40005000, 108000000>;

// USART6 (APB2 @ 216MHz)
using Usart6Hardware = Stm32f7UartHardwarePolicy<0x40011400, 216000000>;

// UART7 (APB1 @ 108MHz)
using Uart7Hardware = Stm32f7UartHardwarePolicy<0x40007800, 108000000>;

// UART8 (APB1 @ 108MHz)
using Uart8Hardware = Stm32f7UartHardwarePolicy<0x40007C00, 108000000>;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief USART1 Simple API
 *
 * Common pins on Nucleo F722ZE:
 * - TX: PA9 (Arduino D8) or PB6 (Arduino D10)
 * - RX: PA10 (Arduino D2) or PB7 (Arduino D4)
 *
 * Usage:
 * @code
 * using TxPin = PinA9;  // USART1_TX
 * using RxPin = PinA10; // USART1_RX
 * auto config = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
 * config.initialize();
 * @endcode
 */
using Usart1 = Uart<PeripheralId::USART1, Usart1Hardware>;

using Usart2 = Uart<PeripheralId::USART2, Usart2Hardware>;
using Usart3 = Uart<PeripheralId::USART3, Usart3Hardware>;
using Uart4 = Uart<PeripheralId::UART4, Uart4Hardware>;
using Uart5 = Uart<PeripheralId::UART5, Uart5Hardware>;
using Usart6 = Uart<PeripheralId::USART6, Usart6Hardware>;
using Uart7 = Uart<PeripheralId::UART7, Uart7Hardware>;
using Uart8 = Uart<PeripheralId::UART8, Uart8Hardware>;

// ============================================================================
// Level 2: Fluent API Type Aliases (Builder Pattern)
// ============================================================================

/**
 * @brief USART1 Fluent API Builder
 *
 * Usage:
 * @code
 * auto config = Usart1Builder()
 *     .with_tx_pin<PinA9>()
 *     .with_rx_pin<PinA10>()
 *     .baudrate(BaudRate{115200})
 *     .parity(UartParity::EVEN)
 *     .data_bits(8)
 *     .stop_bits(1)
 *     .initialize();
 * @endcode
 */
using Usart1Builder = UartBuilder<PeripheralId::USART1, Usart1Hardware>;

using Usart2Builder = UartBuilder<PeripheralId::USART2, Usart2Hardware>;
using Usart3Builder = UartBuilder<PeripheralId::USART3, Usart3Hardware>;
using Uart4Builder = UartBuilder<PeripheralId::UART4, Uart4Hardware>;
using Uart5Builder = UartBuilder<PeripheralId::UART5, Uart5Hardware>;
using Usart6Builder = UartBuilder<PeripheralId::USART6, Usart6Hardware>;
using Uart7Builder = UartBuilder<PeripheralId::UART7, Uart7Hardware>;
using Uart8Builder = UartBuilder<PeripheralId::UART8, Uart8Hardware>;

// ============================================================================
// Level 3: Expert API Type Aliases (Full Control)
// ============================================================================

/**
 * @brief USART1 Expert API Configuration
 *
 * Usage:
 * @code
 * constexpr auto config = Usart1ExpertConfig::standard_115200(
 *     PeripheralId::USART1,
 *     PinId::PA9,   // TX
 *     PinId::PA10   // RX
 * );
 *
 * static_assert(config.is_valid(), config.error_message());
 * auto result = expert::configure(config);
 * @endcode
 */
using Usart1ExpertConfig = UartExpertConfig<Usart1Hardware>;

using Usart2ExpertConfig = UartExpertConfig<Usart2Hardware>;
using Usart3ExpertConfig = UartExpertConfig<Usart3Hardware>;
using Uart4ExpertConfig = UartExpertConfig<Uart4Hardware>;
using Uart5ExpertConfig = UartExpertConfig<Uart5Hardware>;
using Usart6ExpertConfig = UartExpertConfig<Usart6Hardware>;
using Uart7ExpertConfig = UartExpertConfig<Uart7Hardware>;
using Uart8ExpertConfig = UartExpertConfig<Uart8Hardware>;

}  // namespace ucore::hal::stm32f7

/**
 * @example Nucleo F722ZE UART Example
 * @code
 * #include "hal/platform/stm32f7/uart.hpp"
 *
 * using namespace ucore::hal::stm32f7;
 *
 * // Nucleo F722ZE default pins (USART3 on ST-LINK virtual COM port)
 * struct Usart3TxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PD8; }
 * };
 * struct Usart3RxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PD9; }
 * };
 *
 * int main() {
 *     // Simple API - Quick setup
 *     auto uart = Usart3::quick_setup<Usart3TxPin, Usart3RxPin>(BaudRate{115200});
 *     uart.initialize();
 *
 *     const char* msg = "Hello from STM32F722ZE!\n";
 *     uart.write(reinterpret_cast<const uint8_t*>(msg), 24);
 *
 *     while (true) {
 *         // Echo received characters
 *         uint8_t byte;
 *         if (uart.read(&byte, 1).is_ok()) {
 *             uart.write(&byte, 1);
 *         }
 *     }
 * }
 * @endcode
 */
