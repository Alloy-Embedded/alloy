/**
 * @file uart.hpp
 * @brief STM32G0 Platform-Specific UART Type Aliases
 *
 * Combines generic UART APIs with STM32G0 hardware policies to create
 * platform-specific UART instances. This is the integration point between
 * the generic API layer and the vendor-specific hardware policy.
 *
 * Clock Configuration (STM32G071/G0B1):
 * - APB (all USART/LPUART): 64 MHz
 *
 * STM32G0 Features:
 * - Low-power UART (LPUART1) with independent clock
 * - USART1-4 standard peripherals
 * - Advanced features: Auto baud rate, FIFO, wakeup from stop mode
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs (Simple, Fluent, Expert) accept HardwarePolicy as template parameter
 * - Hardware policy provides platform-specific implementation (STM32G0)
 * - Type aliases combine both for convenient usage
 *
 * Architecture Layers:
 * 1. Generic API Layer     -> hal/api/uart_simple.hpp, uart_fluent.hpp, uart_expert.hpp
 * 2. Hardware Policy Layer -> hal/vendors/st/stm32g0/usart_hardware_policy.hpp
 * 3. Integration Layer     -> This file (platform/stm32g0/uart.hpp)
 *
 * Usage Example:
 * @code
 * using namespace ucore::hal::stm32g0;
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
#include "hal/vendors/st/stm32g0/usart_hardware_policy.hpp"

#include "core/types.hpp"

namespace ucore::hal::stm32g0 {

using namespace ucore::core;
using namespace ucore::hal::signals;

// ============================================================================
// Hardware Policy Type Aliases (STM32G0 USART)
// ============================================================================

// USART1 (APB @ 64MHz on STM32G071/G0B1)
using Usart1Hardware = Stm32g0UartHardwarePolicy<0x40013800, 64000000>;

// USART2 (APB @ 64MHz)
using Usart2Hardware = Stm32g0UartHardwarePolicy<0x40004400, 64000000>;

// USART3 (APB @ 64MHz) - Only on STM32G0B1 and higher
using Usart3Hardware = Stm32g0UartHardwarePolicy<0x40004800, 64000000>;

// USART4 (APB @ 64MHz) - Only on STM32G0B1 and higher
using Usart4Hardware = Stm32g0UartHardwarePolicy<0x40004C00, 64000000>;

// LPUART1 (Low-Power UART with independent clock @ 64MHz)
using Lpuart1Hardware = Stm32g0UartHardwarePolicy<0x40008000, 64000000>;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief USART1 Simple API
 *
 * Common pins on Nucleo G071RB / G0B1RE:
 * - TX: PA9 (Arduino D8) or PB6 (Morpho CN10-17)
 * - RX: PA10 (Arduino D2) or PB7 (Morpho CN7-21)
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

/**
 * @brief USART2 Simple API (ST-LINK Virtual COM port on Nucleo)
 *
 * Common pins:
 * - TX: PA2 (ST-LINK VCP, Arduino A7, Morpho CN10-35)
 * - RX: PA3 (ST-LINK VCP, Arduino A2, Morpho CN10-37)
 */
using Usart2 = Uart<PeripheralId::USART2, Usart2Hardware>;

/**
 * @brief USART3 Simple API (STM32G0B1 only)
 *
 * Common pins:
 * - TX: PB10, PC4, PC10
 * - RX: PB11, PC5, PC11
 */
using Usart3 = Uart<PeripheralId::USART3, Usart3Hardware>;

/**
 * @brief USART4 Simple API (STM32G0B1 only)
 *
 * Common pins:
 * - TX: PA0, PC10
 * - RX: PA1, PC11
 */
using Usart4 = Uart<PeripheralId::USART4, Usart4Hardware>;

/**
 * @brief LPUART1 Low-Power UART (all STM32G0)
 *
 * Features:
 * - Can operate in Stop mode
 * - Independent clock source (LSE, HSI, or PCLK)
 * - Wakeup from stop mode capability
 *
 * Common pins:
 * - TX: PA2, PB11, PC1
 * - RX: PA3, PB10, PC0
 */
using Lpuart1 = Uart<PeripheralId::LPUART1, Lpuart1Hardware>;

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
using Usart4Builder = UartBuilder<PeripheralId::USART4, Usart4Hardware>;
using Lpuart1Builder = UartBuilder<PeripheralId::LPUART1, Lpuart1Hardware>;

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
using Usart4ExpertConfig = UartExpertConfig<Usart4Hardware>;
using Lpuart1ExpertConfig = UartExpertConfig<Lpuart1Hardware>;

}  // namespace ucore::hal::stm32g0

/**
 * @example Nucleo G071RB UART Example (ST-LINK VCP)
 * @code
 * #include "hal/platform/stm32g0/uart.hpp"
 *
 * using namespace ucore::hal::stm32g0;
 *
 * // Nucleo G071RB default pins (USART2 on ST-LINK virtual COM port)
 * struct Usart2TxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA2; }
 * };
 * struct Usart2RxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA3; }
 * };
 *
 * int main() {
 *     // Simple API - Quick setup
 *     auto uart = Usart2::quick_setup<Usart2TxPin, Usart2RxPin>(BaudRate{115200});
 *     uart.initialize();
 *
 *     const char* msg = "Hello from STM32G071RB!\n";
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

/**
 * @example Low-Power UART Example
 * @code
 * #include "hal/platform/stm32g0/uart.hpp"
 *
 * using namespace ucore::hal::stm32g0;
 *
 * struct LpuartTxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA2; }
 * };
 * struct LpuartRxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA3; }
 * };
 *
 * int main() {
 *     // LPUART with wakeup capability
 *     auto lpuart = Lpuart1::quick_setup<LpuartTxPin, LpuartRxPin>(BaudRate{9600});
 *     lpuart.initialize();
 *
 *     // LPUART can wake from STOP mode when data arrives
 *     lpuart.enable_wakeup();
 *
 *     while (true) {
 *         lpuart.write_str("Entering stop mode...\n");
 *         // Enter STOP mode - LPUART will wakeup on RX
 *         __WFI();
 *
 *         // Woken up by UART RX
 *         uint8_t data;
 *         if (lpuart.read(&data, 1).is_ok()) {
 *             lpuart.write(&data, 1);  // Echo
 *         }
 *     }
 * }
 * @endcode
 */
