/**
 * @file uart.hpp
 * @brief STM32F4 Platform-Specific UART Type Aliases
 *
 * Combines generic UART APIs with STM32F4 hardware policies to create
 * platform-specific UART instances. This is the integration point between
 * the generic API layer and the vendor-specific hardware policy.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs (Simple, Fluent, Expert) accept HardwarePolicy as template parameter
 * - Hardware policy provides platform-specific implementation (STM32F4)
 * - Type aliases combine both for convenient usage
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/uart_simple.hpp, uart_fluent.hpp, uart_expert.hpp
 * 2. Hardware Policy Layer -> hal/vendors/st/stm32f4/usart_hardware_policy.hpp
 * 3. Integration Layer     -> This file (platform/stm32f4/uart.hpp)
 *
 * Usage Example:
 * @code
 * using namespace alloy::hal::stm32f4;
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
 * @note Part of Phase 10.1: STM32F4 UART Policy
 * @see openspec/changes/modernize-peripheral-architecture/ARCHITECTURE.md
 */

#pragma once

#include "core/types.hpp"
#include "hal/api/uart_simple.hpp"
#include "hal/api/uart_fluent.hpp"
#include "hal/api/uart_expert.hpp"
#include "hal/vendors/st/stm32f4/usart_hardware_policy.hpp"

namespace alloy::hal::stm32f4 {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Hardware Policy Type Aliases (STM32F4 USART)
// ============================================================================

// USART1 (APB2 @ 84MHz)
using Usart1Hardware = Stm32f4UartHardwarePolicy<0x40011000, 84000000>;

// USART2 (APB1 @ 42MHz)
using Usart2Hardware = Stm32f4UartHardwarePolicy<0x40004400, 42000000>;

// USART3 (APB1 @ 42MHz)
using Usart3Hardware = Stm32f4UartHardwarePolicy<0x40004800, 42000000>;

// UART4 (APB1 @ 42MHz)
using Uart4Hardware = Stm32f4UartHardwarePolicy<0x40004C00, 42000000>;

// UART5 (APB1 @ 42MHz)
using Uart5Hardware = Stm32f4UartHardwarePolicy<0x40005000, 42000000>;

// USART6 (APB2 @ 84MHz)
using Usart6Hardware = Stm32f4UartHardwarePolicy<0x40011400, 84000000>;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief USART1 Simple API
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

}  // namespace alloy::hal::stm32f4

/**
 * @example
 * Using the STM32F4 UART with hardware policy:
 *
 * @code
 * #include "hal/uart.hpp"  // Platform dispatch header
 * #include "hal/platform/stm32f4/uart.hpp"
 *
 * using namespace alloy::hal::stm32f4;
 *
 * // Define pin types (from board config)
 * struct Usart1TxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA9; }
 * };
 * struct Usart1RxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA10; }
 * };
 *
 * int main() {
 *     // Simple API
 *     auto uart = Usart1::quick_setup<Usart1TxPin, Usart1RxPin>(BaudRate{115200});
 *     uart.initialize();
 *
 *     const char* msg = "Hello STM32F4!\n";
 *     uart.write(reinterpret_cast<const uint8_t*>(msg), 15);
 * }
 * @endcode
 */
