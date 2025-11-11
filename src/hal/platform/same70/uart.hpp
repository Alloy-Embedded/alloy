/**
 * @file uart.hpp
 * @brief SAME70 Platform-Specific UART Type Aliases
 *
 * Combines generic UART APIs with SAME70 hardware policies to create
 * platform-specific UART instances. This is the integration point between
 * the generic API layer and the vendor-specific hardware policy.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs (Simple, Fluent, Expert) accept HardwarePolicy as template parameter
 * - Hardware policy provides platform-specific implementation (SAME70)
 * - Type aliases combine both for convenient usage
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/uart_simple.hpp, uart_fluent.hpp, uart_expert.hpp
 * 2. Hardware Policy Layer -> hal/vendors/atmel/same70/uart_hardware_policy.hpp
 * 3. Integration Layer     -> This file (platform/same70/uart.hpp)
 *
 * Usage Example:
 * @code
 * using namespace alloy::hal::same70;
 *
 * // Level 1: Simple API - One-liner setup
 * auto uart = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
 * uart.initialize();
 *
 * // Level 2: Fluent API - Builder pattern
 * auto builder = Uart0Builder()
 *     .with_pins<TxPin, RxPin>()
 *     .baudrate(BaudRate{115200})
 *     .standard_8n1()
 *     .initialize();
 *
 * // Level 3: Expert API - Full control
 * constexpr auto config = Uart0ExpertConfig::standard_115200(
 *     PeripheralId::USART0, PinId::PD3, PinId::PD4);
 * expert::configure(config);
 * @endcode
 *
 * @note Part of Phase 8.2: Generic API Integration with Policy
 * @see openspec/changes/modernize-peripheral-architecture/ARCHITECTURE.md
 */

#pragma once

#include "core/types.hpp"
#include "hal/api/uart_simple.hpp"
#include "hal/api/uart_fluent.hpp"
#include "hal/api/uart_expert.hpp"
#include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// ============================================================================
// UART0 Type Aliases (Simple, Fluent, Expert)
// ============================================================================

/// @brief UART0 Simple API - Level 1
using Uart0 = Uart<PeripheralId::USART0, Uart0Hardware>;

/// @brief UART0 Fluent API Builder - Level 2
using Uart0Builder = UartBuilder<PeripheralId::USART0, Uart0Hardware>;

/// @brief UART0 Expert Configuration - Level 3
using Uart0ExpertConfig = UartExpertConfig<Uart0Hardware>;

// ============================================================================
// UART1 Type Aliases (Simple, Fluent, Expert)
// ============================================================================

/// @brief UART1 Simple API - Level 1
using Uart1 = Uart<PeripheralId::USART1, Uart1Hardware>;

/// @brief UART1 Fluent API Builder - Level 2
using Uart1Builder = UartBuilder<PeripheralId::USART1, Uart1Hardware>;

/// @brief UART1 Expert Configuration - Level 3
using Uart1ExpertConfig = UartExpertConfig<Uart1Hardware>;

// ============================================================================
// UART2 Type Aliases (Simple, Fluent, Expert)
// ============================================================================

/// @brief UART2 Simple API - Level 1
using Uart2 = Uart<PeripheralId::UART2, Uart2Hardware>;

/// @brief UART2 Fluent API Builder - Level 2
using Uart2Builder = UartBuilder<PeripheralId::UART2, Uart2Hardware>;

/// @brief UART2 Expert Configuration - Level 3
using Uart2ExpertConfig = UartExpertConfig<Uart2Hardware>;

// ============================================================================
// UART3 Type Aliases (Simple, Fluent, Expert)
// ============================================================================

/// @brief UART3 Simple API - Level 1
using Uart3 = Uart<PeripheralId::UART3, Uart3Hardware>;

/// @brief UART3 Fluent API Builder - Level 2
using Uart3Builder = UartBuilder<PeripheralId::UART3, Uart3Hardware>;

/// @brief UART3 Expert Configuration - Level 3
using Uart3ExpertConfig = UartExpertConfig<Uart3Hardware>;

// ============================================================================
// UART4 Type Aliases (Simple, Fluent, Expert)
// ============================================================================

/// @brief UART4 Simple API - Level 1
using Uart4 = Uart<PeripheralId::UART4, Uart4Hardware>;

/// @brief UART4 Fluent API Builder - Level 2
using Uart4Builder = UartBuilder<PeripheralId::UART4, Uart4Hardware>;

/// @brief UART4 Expert Configuration - Level 3
using Uart4ExpertConfig = UartExpertConfig<Uart4Hardware>;

}  // namespace alloy::hal::same70

/**
 * @example Level 1: Simple API Usage
 *
 * The simplest way to setup UART with one line:
 *
 * @code
 * #include "hal/platform/same70/uart.hpp"
 * #include "hal/platform/same70/gpio.hpp"
 *
 * using namespace alloy::hal::same70;
 *
 * // Define pins
 * using UartTx = GpioPin<PIOD_BASE, 3>;  // PD3 - USART0 TX
 * using UartRx = GpioPin<PIOD_BASE, 4>;  // PD4 - USART0 RX
 *
 * int main() {
 *     // One-liner UART setup
 *     auto uart = Uart0::quick_setup<UartTx, UartRx>(BaudRate{115200});
 *     uart.initialize();
 *
 *     // Use it
 *     // uart.write(...);
 * }
 * @endcode
 */

/**
 * @example Level 2: Fluent API Usage
 *
 * Builder pattern with readable method chaining:
 *
 * @code
 * #include "hal/platform/same70/uart.hpp"
 *
 * using namespace alloy::hal::same70;
 *
 * int main() {
 *     auto result = Uart0Builder()
 *         .with_pins<UartTx, UartRx>()
 *         .baudrate(BaudRate{115200})
 *         .standard_8n1()
 *         .initialize();
 *
 *     if (result.is_ok()) {
 *         auto config = result.value();
 *         config.apply();
 *     }
 * }
 * @endcode
 */

/**
 * @example Level 3: Expert API Usage
 *
 * Complete control with compile-time validation:
 *
 * @code
 * #include "hal/platform/same70/uart.hpp"
 *
 * using namespace alloy::hal::same70;
 *
 * // Compile-time configuration
 * constexpr auto uart_config = Uart0ExpertConfig::standard_115200(
 *     PeripheralId::USART0,
 *     PinId::PD3,  // TX
 *     PinId::PD4   // RX
 * );
 *
 * // Validate at compile-time
 * static_assert(uart_config.is_valid(), uart_config.error_message());
 *
 * int main() {
 *     // Apply configuration
 *     auto result = expert::configure(uart_config);
 *
 *     if (result.is_ok()) {
 *         // UART is ready to use
 *     }
 * }
 * @endcode
 */
