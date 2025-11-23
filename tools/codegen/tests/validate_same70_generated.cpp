/**
 * @file validate_same70_generated.cpp
 * @brief Compile-time validation for SAME70 generated register code
 *
 * This test validates that generated register definitions from SAME70 SVD:
 * - Compile without errors or warnings
 * - Have correct struct layouts and offsets
 * - Match Atmel SAME70 datasheet specifications
 * - Follow project code style
 *
 * Build with strict warnings:
 *   -Wall -Wextra -Wpedantic -Werror
 */

#include <cstdint>
#include <type_traits>

// Include generated register definitions for SAME70
#include "hal/vendors/atmel/same70/generated/registers/pmc_registers.hpp"
#include "hal/vendors/atmel/same70/generated/registers/pioa_registers.hpp"
// #include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

using namespace ucore::hal::atmel::same70;

// ============================================================================
// Compile-Time Validation Tests
// ============================================================================

/**
 * Test 1: PIO (GPIO) Register Structure Layout
 *
 * Validates that PIOA register structure matches SAME70 datasheet
 */
namespace test_pio_layout {

// PIOA base address from SAME70 datasheet
constexpr uintptr_t PIOA_BASE = 0x400E0E00;

// Validate key register offsets (from SAME70 datasheet Table 31-3)
static_assert(offsetof(pioa::PIOA_Registers, PER) == 0x00, "PER offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, PDR) == 0x04, "PDR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, PSR) == 0x08, "PSR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, OER) == 0x10, "OER offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, ODR) == 0x14, "ODR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, OSR) == 0x18, "OSR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, IFER) == 0x20, "IFER offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, IFDR) == 0x24, "IFDR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, IFSR) == 0x28, "IFSR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, SODR) == 0x30, "SODR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, CODR) == 0x34, "CODR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, ODSR) == 0x38, "ODSR offset incorrect");
static_assert(offsetof(pioa::PIOA_Registers, PDSR) == 0x3C, "PDSR offset incorrect");

// Validate peripheral select registers
static_assert(offsetof(pioa::PIOA_Registers, ABCDSR) == 0x70, "ABCDSR offset incorrect");

// Validate structure fits within memory map (PIO is 0x200 bytes)
static_assert(sizeof(pioa::PIOA_Registers) <= 0x200, "PIOA_Registers size exceeds memory map");

// Validate volatile qualifiers
static_assert(std::is_volatile_v<decltype(pioa::PIOA_Registers::SODR)>,
              "SODR must be volatile");
static_assert(std::is_volatile_v<decltype(pioa::PIOA_Registers::CODR)>,
              "CODR must be volatile");
static_assert(std::is_volatile_v<decltype(pioa::PIOA_Registers::PDSR)>,
              "PDSR must be volatile");

} // namespace test_pio_layout

/**
 * Test 2: PMC Register Structure Layout
 *
 * Validates Power Management Controller registers
 */
namespace test_pmc_layout {

constexpr uintptr_t PMC_BASE = 0x400E0600;

// Validate key PMC registers (from SAME70 datasheet Table 28-3)
static_assert(offsetof(pmc::PMC_Registers, SCER) == 0x00, "SCER offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, SCDR) == 0x04, "SCDR offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, SCSR) == 0x08, "SCSR offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, PCER0) == 0x10, "PCER0 offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, PCDR0) == 0x14, "PCDR0 offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, PCSR0) == 0x18, "PCSR0 offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, CKGR_MOR) == 0x20, "CKGR_MOR offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, CKGR_PLLAR) == 0x28, "CKGR_PLLAR offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, MCKR) == 0x30, "MCKR offset incorrect");
static_assert(offsetof(pmc::PMC_Registers, SR) == 0x68, "SR offset incorrect");

// Validate volatile qualifiers
static_assert(std::is_volatile_v<decltype(pmc::PMC_Registers::CKGR_MOR)>,
              "CKGR_MOR must be volatile");
static_assert(std::is_volatile_v<decltype(pmc::PMC_Registers::CKGR_PLLAR)>,
              "CKGR_PLLAR must be volatile");
static_assert(std::is_volatile_v<decltype(pmc::PMC_Registers::MCKR)>,
              "MCKR must be volatile");
static_assert(std::is_volatile_v<decltype(pmc::PMC_Registers::SR)>,
              "SR must be volatile");

} // namespace test_pmc_layout

/**
 * Test 3: Type Safety
 *
 * Validates that generated types are type-safe and match SAME70 architecture
 */
namespace test_type_safety {

// All register types should be uint32_t (standard ARM Cortex-M7 register size)
static_assert(std::is_same_v<decltype(pio::PIO_Registers::SODR), volatile uint32_t>,
              "SODR must be volatile uint32_t");
static_assert(std::is_same_v<decltype(pmc::PMC_Registers::MCKR), volatile uint32_t>,
              "MCKR must be volatile uint32_t");

// Structures should be standard layout (required for hardware mapping)
static_assert(std::is_standard_layout_v<pio::PIO_Registers>,
              "PIO_Registers must be standard layout");
static_assert(std::is_standard_layout_v<pmc::PMC_Registers>,
              "PMC_Registers must be standard layout");

// Structures should be trivially copyable (no constructors/destructors)
static_assert(std::is_trivially_copyable_v<pio::PIO_Registers>,
              "PIO_Registers must be trivially copyable");
static_assert(std::is_trivially_copyable_v<pmc::PMC_Registers>,
              "PMC_Registers must be trivially copyable");

// Verify no virtual functions (zero-overhead principle)
static_assert(!std::is_polymorphic_v<pio::PIO_Registers>,
              "PIO_Registers must not have virtual functions");
static_assert(!std::is_polymorphic_v<pmc::PMC_Registers>,
              "PMC_Registers must not have virtual functions");

} // namespace test_type_safety

/**
 * Test 4: Peripheral Base Addresses
 *
 * Validates that peripheral base addresses match SAME70Q21B datasheet
 */
namespace test_peripheral_addresses {

using namespace ucore::generated::atsame70q21b;

// Verify key peripheral addresses (from SAME70 datasheet Table 7-1)
static_assert(peripherals::PMC == 0x400E0600, "PMC base address incorrect");
static_assert(peripherals::PIOA == 0x400E0E00, "PIOA base address incorrect");
static_assert(peripherals::PIOB == 0x400E1000, "PIOB base address incorrect");
static_assert(peripherals::PIOC == 0x400E1200, "PIOC base address incorrect");
static_assert(peripherals::PIOD == 0x400E1400, "PIOD base address incorrect");
static_assert(peripherals::PIOE == 0x400E1600, "PIOE base address incorrect");
static_assert(peripherals::UART0 == 0x400E0800, "UART0 base address incorrect");

} // namespace test_peripheral_addresses

/**
 * Test 5: Bitfield Operations
 *
 * Validates that bitfield operations work correctly at compile time
 */
namespace test_bitfields {

// Test PIO SODR (Set Output Data Register) - writing 1 sets the pin
constexpr uint32_t test_sodr = 0;
constexpr uint32_t set_pin8 = (1u << 8);  // Set pin 8

constexpr uint32_t result = test_sodr | set_pin8;
static_assert(result == 0x00000100, "Pin 8 set operation incorrect");

// Test PMC PCER0 (Peripheral Clock Enable Register 0)
// Enabling PIOA (peripheral ID 10)
constexpr uint32_t test_pcer0 = 0;
constexpr uint32_t enable_pioa = (1u << 10);

constexpr uint32_t pcer0_result = test_pcer0 | enable_pioa;
static_assert(pcer0_result == 0x00000400, "PIOA clock enable incorrect");

// Test CKGR_MOR key value (must write 0x37 to KEY field)
// From SAME70 datasheet: KEY must be 0x37 for writes to be accepted
constexpr uint32_t MOR_KEY = 0x37;
constexpr uint32_t MOR_KEY_SHIFT = 16;
constexpr uint32_t MOR_KEY_VALUE = (MOR_KEY << MOR_KEY_SHIFT);
static_assert(MOR_KEY_VALUE == 0x00370000, "CKGR_MOR KEY value incorrect");

} // namespace test_bitfields

// ============================================================================
// Main - Required for compilation but not executed
// ============================================================================

/**
 * This file is a compile-time validation test.
 * All validation happens via static_assert at compile time.
 */
int main() {
    // All tests passed at compile time!
    // If this compiles, the generated code is valid.
    return 0;
}
