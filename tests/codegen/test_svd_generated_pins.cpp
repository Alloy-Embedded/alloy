/**
 * Test for SVD-generated pin headers
 * Verifies that pin headers generated from SVD files compile correctly
 */

// Test multiple variants
#include "../../src/hal/st/stm32f103/generated/stm32f103c8/pins.hpp"
#include "../../src/hal/st/stm32f103/generated/stm32f103c8/traits.hpp"
#include "../../src/hal/st/stm32f103/generated/stm32f103cb/pins.hpp"
#include "../../src/hal/st/stm32f103/generated/stm32f103cb/traits.hpp"
#include "../../src/hal/st/stm32f103/generated/stm32f103re/pins.hpp"
#include "../../src/hal/st/stm32f103/generated/stm32f103re/traits.hpp"

#include <cstdint>

// Test STM32F103C8 (LQFP48)
namespace test_c8 {
    using namespace alloy::hal::stm32f103::stm32f103c8;

    // Test basic pin constants
    static_assert(pins::PA0 == 0, "PA0 should be 0");
    static_assert(pins::PA15 == 15, "PA15 should be 15");
    static_assert(pins::PB0 == 16, "PB0 should be 16");
    static_assert(pins::PB15 == 31, "PB15 should be 31");
    static_assert(pins::PC13 == 45, "PC13 should be 45");
    static_assert(pins::PC14 == 46, "PC14 should be 46");
    static_assert(pins::PC15 == 47, "PC15 should be 47");
    static_assert(pins::PD0 == 48, "PD0 should be 48");
    static_assert(pins::PD1 == 49, "PD1 should be 49");

    // Test pin validation
    static_assert(pins::is_valid_pin_v<pins::PA0>, "PA0 should be valid");
    static_assert(pins::is_valid_pin_v<pins::PB5>, "PB5 should be valid");
    static_assert(pins::is_valid_pin_v<pins::PC13>, "PC13 should be valid");

    // Test pin count
    static_assert(pins::TOTAL_PIN_COUNT == 37, "C8 should have 37 GPIO pins");
    static_assert(pins::GPIO_PORT_COUNT == 4, "C8 should have 4 GPIO ports");

    // Test traits
    static_assert(Traits::PIN_COUNT == 37, "C8 package should have 37 pins");
    static_assert(Traits::FLASH_SIZE == 64 * 1024, "C8 should have 64KB flash");
    static_assert(Traits::SRAM_SIZE == 20 * 1024, "C8 should have 20KB SRAM");

    // Test peripheral counts (from SVD)
    static_assert(Traits::Peripherals::UART_COUNT == 5, "Should detect 5 UARTs from SVD");
    static_assert(Traits::Peripherals::I2C_COUNT == 2, "Should detect 2 I2C from SVD");
    static_assert(Traits::Peripherals::SPI_COUNT == 3, "Should detect 3 SPI from SVD");
    static_assert(Traits::Peripherals::ADC_COUNT == 3, "Should detect 3 ADC from SVD");

    // Test concepts (C++20)
    static_assert(pins::ValidPin<pins::PA0>);
    static_assert(pins::ValidPin<pins::PC13>);
}

// Test STM32F103CB (LQFP48 with more flash)
namespace test_cb {
    using namespace alloy::hal::stm32f103::stm32f103cb;

    // Test traits differences
    static_assert(Traits::PIN_COUNT == 37, "CB package should have 37 pins");
    static_assert(Traits::FLASH_SIZE == 128 * 1024, "CB should have 128KB flash");
    static_assert(Traits::SRAM_SIZE == 20 * 1024, "CB should have 20KB SRAM");
}

// Test STM32F103RE (LQFP64)
namespace test_re {
    using namespace alloy::hal::stm32f103::stm32f103re;

    // Test pin count for larger package
    static_assert(pins::TOTAL_PIN_COUNT == 67, "RE should have 67 GPIO pins");
    static_assert(pins::GPIO_PORT_COUNT == 5, "RE should have 5 GPIO ports (A-E)");

    // Test traits
    static_assert(Traits::PIN_COUNT == 67, "RE package should have 67 pins");
    static_assert(Traits::FLASH_SIZE == 512 * 1024, "RE should have 512KB flash");
    static_assert(Traits::SRAM_SIZE == 64 * 1024, "RE should have 64KB SRAM");

    // Test additional pins available in LQFP64
    static_assert(pins::PC0 == 32, "PC0 should be available in LQFP64");
    static_assert(pins::PD2 == 50, "PD2 should be available in LQFP64");
    static_assert(pins::PE0 == 64, "PE0 should be available in LQFP64");
    static_assert(pins::PE15 == 79, "PE15 should be available in LQFP64");
}

int main() {
    // Runtime tests
    using namespace alloy::hal::stm32f103::stm32f103c8;

    // Test constexpr pin validation at runtime
    constexpr bool pa0_valid = pins::is_valid_pin_v<pins::PA0>;
    constexpr bool pb5_valid = pins::is_valid_pin_v<5>;

    // Test traits at runtime
    const char* device_name = Traits::DEVICE_NAME;
    const char* vendor = Traits::VENDOR;
    uint32_t flash_size = Traits::FLASH_SIZE;

    return 0;
}
