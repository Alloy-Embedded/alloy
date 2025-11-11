/**
 * @file uart.hpp
 * @brief STM32F1 Platform-Specific UART Type Aliases
 *
 * STM32F1 (Blue Pill) UART/USART integration with policy-based design.
 * 
 * Clock Configuration (STM32F103):
 * - APB2 (USART1): 72 MHz
 * - APB1 (USART2/3): 36 MHz
 *
 * @note Part of Phase 10.3: STM32F1 Support
 */

#pragma once

#include "core/types.hpp"
#include "hal/api/uart_simple.hpp"
#include "hal/api/uart_fluent.hpp"
#include "hal/api/uart_expert.hpp"
#include "hal/vendors/st/stm32f1/usart_hardware_policy.hpp"

namespace alloy::hal::stm32f1 {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Hardware Policy Type Aliases (STM32F1 USART)
// ============================================================================

// USART1 (APB2 @ 72MHz)
using Usart1Hardware = Stm32f1UartHardwarePolicy<0x40013800, 72000000>;

// USART2 (APB1 @ 36MHz)
using Usart2Hardware = Stm32f1UartHardwarePolicy<0x40004400, 36000000>;

// USART3 (APB1 @ 36MHz)
using Usart3Hardware = Stm32f1UartHardwarePolicy<0x40004800, 36000000>;

// ============================================================================
// Level 1: Simple API Type Aliases
// ============================================================================

/**
 * @brief USART1 Simple API (Blue Pill default)
 *
 * Common pins on Blue Pill:
 * - TX: PA9
 * - RX: PA10
 */
using Usart1 = Uart<PeripheralId::USART1, Usart1Hardware>;
using Usart2 = Uart<PeripheralId::USART2, Usart2Hardware>;
using Usart3 = Uart<PeripheralId::USART3, Usart3Hardware>;

// ============================================================================
// Level 2: Fluent API Type Aliases (Builder Pattern)
// ============================================================================

using Usart1Builder = UartBuilder<PeripheralId::USART1, Usart1Hardware>;
using Usart2Builder = UartBuilder<PeripheralId::USART2, Usart2Hardware>;
using Usart3Builder = UartBuilder<PeripheralId::USART3, Usart3Hardware>;

// ============================================================================
// Level 3: Expert API Type Aliases (Full Control)
// ============================================================================

using Usart1ExpertConfig = UartExpertConfig<Usart1Hardware>;
using Usart2ExpertConfig = UartExpertConfig<Usart2Hardware>;
using Usart3ExpertConfig = UartExpertConfig<Usart3Hardware>;

}  // namespace alloy::hal::stm32f1

/**
 * @example Blue Pill UART Example
 * @code
 * #include "hal/platform/stm32f1/uart.hpp"
 *
 * using namespace alloy::hal::stm32f1;
 *
 * // Blue Pill default pins
 * struct Usart1TxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA9; }
 * };
 * struct Usart1RxPin {
 *     static constexpr PinId get_pin_id() { return PinId::PA10; }
 * };
 *
 * int main() {
 *     auto uart = Usart1::quick_setup<Usart1TxPin, Usart1RxPin>(BaudRate{115200});
 *     uart.initialize();
 *
 *     const char* msg = "Hello from Blue Pill!\n";
 *     uart.write(reinterpret_cast<const uint8_t*>(msg), 22);
 * }
 * @endcode
 */
