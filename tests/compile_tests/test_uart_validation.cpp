/**
 * @file test_uart_validation.cpp
 * @brief Compile-time validation tests for UART/USART peripherals
 *
 * This file contains tests to verify that invalid USART instances are rejected
 * at compile time. These are NOT runtime tests - they test compile-time
 * validation using static_assert.
 *
 * To test validation failure:
 * 1. Uncomment one of the "SHOULD_FAIL" test cases
 * 2. Try to compile - it should fail with a clear error message
 * 3. Re-comment the test case
 */

#ifdef ALLOY_MCU  // Only compile when building for real hardware

#include "hal/st/stm32f1/uart.hpp"

using namespace alloy::hal::stm32f1;

// ============================================================================
// Valid USART Tests (these should compile successfully)
// ============================================================================

// Test valid USART instances on STM32F103C8
// STM32F103C8 has USART1, USART2, USART3, UART4, UART5 (from database)
void test_valid_usart_instances() {
    UartDevice<UsartId::USART1> usart1;  // Valid
    UartDevice<UsartId::USART2> usart2;  // Valid
    UartDevice<UsartId::USART3> usart3;  // Valid
}

// ============================================================================
// Invalid USART Tests (uncomment to test compile-time validation)
// ============================================================================

// Note: The current code only supports USART1/2/3 in the enum.
// If we add UART4/UART5 to the enum later, we can test validation for MCUs
// that don't have those peripherals.

// Example for future testing:
// If we had an STM32F103C4 (smaller chip with only USART1):
// SHOULD_FAIL: USART3 doesn't exist on smaller variants
// void test_invalid_usart_on_small_chip() {
//     UartDevice<UsartId::USART3> usart3;  // Should fail
// }

#endif // ALLOY_MCU
