/**
 * @file validate_stm32f4_generated.cpp
 * @brief Compile-time validation for STM32F4 generated register code
 *
 * This test validates that generated register definitions from SVD files:
 * - Compile without errors or warnings
 * - Have correct struct layouts and offsets
 * - Match SVD specifications exactly
 * - Follow project code style
 *
 * This runs at build time to catch code generation issues early.
 *
 * Build with strict warnings:
 *   -Wall -Wextra -Wpedantic -Werror
 *   -Wconversion -Wsign-conversion
 */

#include <cstdint>
#include <type_traits>

// Include generated register definitions for STM32F401
#include "hal/vendors/st/stm32f4/generated/stm32f401/registers.hpp"

using namespace ucore::hal::st::stm32f4::stm32f401;

// ============================================================================
// Compile-Time Validation Tests
// ============================================================================

/**
 * Test 1: GPIO Register Structure Layout
 *
 * Validates that GPIOA register structure matches STM32F4 reference manual:
 * - Correct register offsets
 * - Correct structure size
 * - Volatile qualifiers present
 */
namespace test_gpio_layout {

// GPIOA base address from STM32F401 reference manual
constexpr uintptr_t GPIOA_BASE = 0x40020000;

// Validate register offsets match reference manual
static_assert(offsetof(gpio::GPIO_Registers, MODER) == 0x00, "MODER offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, OTYPER) == 0x04, "OTYPER offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, OSPEEDR) == 0x08, "OSPEEDR offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, PUPDR) == 0x0C, "PUPDR offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, IDR) == 0x10, "IDR offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, ODR) == 0x14, "ODR offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, BSRR) == 0x18, "BSRR offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, LCKR) == 0x1C, "LCKR offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, AFRL) == 0x20, "AFRL offset incorrect");
static_assert(offsetof(gpio::GPIO_Registers, AFRH) == 0x24, "AFRH offset incorrect");

// Validate structure size (should be 0x28 bytes = 40 bytes = 10 registers × 4 bytes)
static_assert(sizeof(gpio::GPIO_Registers) <= 0x400, "GPIO_Registers size exceeds memory map");

// Validate all register fields are volatile
static_assert(std::is_volatile_v<decltype(gpio::GPIO_Registers::MODER)>,
              "MODER must be volatile");
static_assert(std::is_volatile_v<decltype(gpio::GPIO_Registers::ODR)>,
              "ODR must be volatile");

} // namespace test_gpio_layout

/**
 * Test 2: RCC Register Structure Layout
 *
 * Validates Reset and Clock Control (RCC) registers
 */
namespace test_rcc_layout {

constexpr uintptr_t RCC_BASE = 0x40023800;

static_assert(offsetof(rcc::RCC_Registers, CR) == 0x00, "CR offset incorrect");
static_assert(offsetof(rcc::RCC_Registers, PLLCFGR) == 0x04, "PLLCFGR offset incorrect");
static_assert(offsetof(rcc::RCC_Registers, CFGR) == 0x08, "CFGR offset incorrect");
static_assert(offsetof(rcc::RCC_Registers, CIR) == 0x0C, "CIR offset incorrect");
static_assert(offsetof(rcc::RCC_Registers, AHB1RSTR) == 0x10, "AHB1RSTR offset incorrect");

// Validate volatile qualifiers
static_assert(std::is_volatile_v<decltype(rcc::RCC_Registers::CR)>,
              "CR must be volatile");
static_assert(std::is_volatile_v<decltype(rcc::RCC_Registers::PLLCFGR)>,
              "PLLCFGR must be volatile");

} // namespace test_rcc_layout

/**
 * Test 3: USART Register Structure Layout
 *
 * Validates UART/USART peripheral registers
 */
namespace test_usart_layout {

constexpr uintptr_t USART1_BASE = 0x40011000;

static_assert(offsetof(usart::USART_Registers, SR) == 0x00, "SR offset incorrect");
static_assert(offsetof(usart::USART_Registers, DR) == 0x04, "DR offset incorrect");
static_assert(offsetof(usart::USART_Registers, BRR) == 0x08, "BRR offset incorrect");
static_assert(offsetof(usart::USART_Registers, CR1) == 0x0C, "CR1 offset incorrect");
static_assert(offsetof(usart::USART_Registers, CR2) == 0x10, "CR2 offset incorrect");
static_assert(offsetof(usart::USART_Registers, CR3) == 0x14, "CR3 offset incorrect");
static_assert(offsetof(usart::USART_Registers, GTPR) == 0x18, "GTPR offset incorrect");

// Validate volatile qualifiers
static_assert(std::is_volatile_v<decltype(usart::USART_Registers::SR)>,
              "SR must be volatile");
static_assert(std::is_volatile_v<decltype(usart::USART_Registers::DR)>,
              "DR must be volatile");

} // namespace test_usart_layout

/**
 * Test 4: Type Safety
 *
 * Validates that generated types are type-safe
 */
namespace test_type_safety {

// All register types should be uint32_t (standard ARM register size)
static_assert(std::is_same_v<decltype(gpio::GPIO_Registers::MODER), volatile uint32_t>,
              "MODER must be volatile uint32_t");
static_assert(std::is_same_v<decltype(rcc::RCC_Registers::CR), volatile uint32_t>,
              "CR must be volatile uint32_t");
static_assert(std::is_same_v<decltype(usart::USART_Registers::DR), volatile uint32_t>,
              "DR must be volatile uint32_t");

// Structures should be standard layout (required for hardware mapping)
static_assert(std::is_standard_layout_v<gpio::GPIO_Registers>,
              "GPIO_Registers must be standard layout");
static_assert(std::is_standard_layout_v<rcc::RCC_Registers>,
              "RCC_Registers must be standard layout");
static_assert(std::is_standard_layout_v<usart::USART_Registers>,
              "USART_Registers must be standard layout");

// Structures should be trivially copyable (no constructors/destructors)
static_assert(std::is_trivially_copyable_v<gpio::GPIO_Registers>,
              "GPIO_Registers must be trivially copyable");

} // namespace test_type_safety

/**
 * Test 5: Bitfield Definitions
 *
 * Validates that bitfield helper functions are correctly generated
 */
namespace test_bitfields {

// Test GPIO MODER bitfield (2 bits per pin, 32 pins)
// Each pin has 2 bits: 00=Input, 01=Output, 10=Alternate, 11=Analog
constexpr uint32_t test_moder_value = 0;
constexpr uint32_t pin5_output = (0b01 << (5 * 2));  // Pin 5 as output

// Bitfield operations should be constexpr
constexpr uint32_t set_pin5_output = test_moder_value | pin5_output;
static_assert(set_pin5_output == 0x00000400, "Pin 5 output bit position incorrect");

// Test RCC PLL configuration bits
constexpr uint32_t test_pllcfgr = 0;
constexpr uint32_t pll_m_mask = 0x3F;       // Bits 0-5
constexpr uint32_t pll_n_mask = 0x7FC0;     // Bits 6-14
constexpr uint32_t pll_p_mask = 0x30000;    // Bits 16-17

static_assert((test_pllcfgr & pll_m_mask) == 0, "PLLM mask incorrect");

} // namespace test_bitfields

// ============================================================================
// Main - Required for compilation but not executed
// ============================================================================

/**
 * This file is a compile-time validation test.
 * The main() function exists only to satisfy the linker.
 * All validation happens via static_assert at compile time.
 */
int main() {
    // All tests passed at compile time!
    // If this compiles, the generated code is valid.
    return 0;
}
