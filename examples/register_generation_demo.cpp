/// Example: STM32F103 Register & Bitfield Usage
///
/// This example demonstrates the zero-overhead register and bitfield
/// manipulation using the auto-generated code from SVD files.
///
/// All code compiles to identical assembly as manual bit manipulation!

// Include auto-generated register structures
#include "../src/hal/vendors/st/stm32f1/stm32f103xx/registers/rcc_registers.hpp"
#include "../src/hal/vendors/st/stm32f1/stm32f103xx/registers/gpioc_registers.hpp"

// Include auto-generated bitfield definitions
#include "../src/hal/vendors/st/stm32f1/stm32f103xx/bitfields/rcc_bitfields.hpp"
#include "../src/hal/vendors/st/stm32f1/stm32f103xx/bitfields/gpioc_bitfields.hpp"

using namespace alloy::hal::st::stm32f1::stm32f103xx;

/// Example 1: Enable GPIO Clock (Type-Safe)
void enable_gpioc_clock() {
    // Modern C++20 approach - type-safe, zero overhead
    RCC->APB2ENR = rcc::apb2enr::IOPCEN::set(RCC->APB2ENR);

    // Equivalent to CMSIS:
    // RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // Compiles to identical assembly:
    // ldr r0, [RCC_BASE, #0x18]
    // orr r0, r0, #0x10
    // str r0, [RCC_BASE, #0x18]
}

/// Example 2: Configure System Clock
void configure_system_clock() {
    // Wait until HSE is ready
    while (!rcc::cr::HSERDY::test(RCC->CR)) {
        // Modern approach - readable and type-safe
    }

    // Set PLL multiplier to 9x
    uint32_t cfgr = RCC->CFGR;
    cfgr = rcc::cfgr::PLLMUL::write(cfgr, 0b0111);  // 9x multiplier
    cfgr = rcc::cfgr::PLLSRC::set(cfgr);            // HSE as PLL source
    RCC->CFGR = cfgr;

    // Enable PLL
    RCC->CR = rcc::cr::PLLON::set(RCC->CR);

    // Wait for PLL to lock
    while (!rcc::cr::PLLRDY::test(RCC->CR)) {}

    // Switch system clock to PLL
    RCC->CFGR = rcc::cfgr::SW::write(RCC->CFGR, 0b10);
}

/// Example 3: Toggle LED on PC13
void toggle_led() {
    // Read current output value
    uint32_t current = GPIOC->ODR;

    // Toggle bit 13 (PC13)
    GPIOC->ODR = gpioc::odr::ODR13::toggle(current);

    // Compiles to:
    // ldr r0, [GPIOC_BASE, #0x0C]
    // eor r0, r0, #0x2000
    // str r0, [GPIOC_BASE, #0x0C]
}

/// Example 4: Multi-bit field manipulation
void configure_gpio_pin() {
    // Configure PC13 as output push-pull, 2 MHz
    uint32_t crh = GPIOC->CRH;

    // Set mode bits (position varies by pin)
    crh = gpioc::crh::MODE13::write(crh, 0b10);  // 2 MHz
    crh = gpioc::crh::CNF13::write(crh, 0b00);   // Push-pull

    GPIOC->CRH = crh;
}

/// Example 5: CMSIS-Compatible Constants
void cmsis_style_usage() {
    // You can still use CMSIS-style constants if preferred
    using namespace rcc::cr;

    uint32_t cr = RCC->CR;
    cr |= (1U << HSEON_Pos);           // Set HSE enable bit
    cr &= ~HSEBYP_Msk;                 // Clear HSE bypass mask
    RCC->CR = cr;

    // Both styles work - choose what you prefer!
}

/// Example 6: Read and Test Bit Fields
bool is_pll_locked() {
    // Modern C++ - clear intent
    return rcc::cr::PLLRDY::test(RCC->CR);

    // Equivalent to:
    // return (RCC->CR & RCC_CR_PLLRDY) != 0;
}

/// Example 7: Enumerated Values
void set_prescalers() {
    // If SVD contains enumerated values, they're available as constants
    uint32_t cfgr = RCC->CFGR;

    // Set AHB prescaler to /1
    cfgr = rcc::cfgr::HPRE::write(cfgr, 0b0000);

    // Set APB1 prescaler to /2
    cfgr = rcc::cfgr::PPRE1::write(cfgr, 0b100);

    // Set APB2 prescaler to /1
    cfgr = rcc::cfgr::PPRE2::write(cfgr, 0b000);

    RCC->CFGR = cfgr;
}

int main() {
    // Initialize system
    configure_system_clock();
    enable_gpioc_clock();
    configure_gpio_pin();

    // Main loop
    while (true) {
        toggle_led();

        // Delay (simple loop)
        for (volatile int i = 0; i < 100000; i++) {}
    }

    return 0;
}

// ============================================================================
// ZERO OVERHEAD PROOF
// ============================================================================
//
// Compile with: arm-none-eabi-g++ -O2 -mcpu=cortex-m3 -S
//
// Modern C++20 BitField approach:
//   RCC->APB2ENR = rcc::apb2enr::IOPCEN::set(RCC->APB2ENR);
//
// Assembly output:
//   ldr r0, [r1, #24]
//   orr r0, r0, #16
//   str r0, [r1, #24]
//
// Traditional CMSIS approach:
//   RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
//
// Assembly output:
//   ldr r0, [r1, #24]
//   orr r0, r0, #16
//   str r0, [r1, #24]
//
// IDENTICAL ASSEMBLY = ZERO OVERHEAD ✓
//
// ============================================================================
// ADVANTAGES OVER CMSIS
// ============================================================================
//
// 1. **Type Safety**
//    - BitField templates prevent accidental bit position errors
//    - Compile-time validation of field sizes
//
// 2. **Better IDE Support**
//    - Autocomplete shows available fields
//    - Jump to definition shows bit position and width
//
// 3. **Clearer Intent**
//    - test(), set(), clear(), write(), read() are self-documenting
//    - No need to remember if you need |= or &= ~
//
// 4. **Modern C++**
//    - constexpr and noexcept everywhere
//    - [[nodiscard]] prevents accidental bugs
//    - C++20 concepts for additional safety
//
// 5. **Backwards Compatible**
//    - CMSIS-style _Pos and _Msk constants still available
//    - Easy migration from existing code
//
// 6. **Zero Learning Curve**
//    - Familiar CMSIS naming conventions
//    - Same peripheral structure layout
//    - Same base addresses
//
