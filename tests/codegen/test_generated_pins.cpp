/**
 * Test generated pin definitions
 *
 * Verifies that generated pin headers:
 * 1. Define correct pin constants
 * 2. Provide compile-time validation
 * 3. Have zero runtime overhead
 */

#include <hal/st/stm32f1/generated/stm32f103c8/pins.hpp>
#include <hal/st/stm32f1/generated/stm32f103c8/traits.hpp>

// Test namespace imports
using namespace alloy::hal::stm32f1::stm32f103c8;

// Compile-time tests
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
static_assert(pins::is_valid_pin_v<pins::PC13>, "PC13 should be valid");
static_assert(pins::is_valid_pin_v<pins::PD1>, "PD1 should be valid");

// Test pin counts
static_assert(pins::TOTAL_PIN_COUNT == 37, "Should have 37 pins");
static_assert(pins::GPIO_PORT_COUNT == 4, "Should have 4 ports");

// Test traits
static_assert(Traits::FLASH_SIZE == 64 * 1024, "Should have 64KB flash");
static_assert(Traits::SRAM_SIZE == 20 * 1024, "Should have 20KB SRAM");
static_assert(Traits::Peripherals::UART_COUNT == 3, "Should have 3 UARTs");
static_assert(Traits::Peripherals::I2C_COUNT == 2, "Should have 2 I2C");
static_assert(Traits::Peripherals::SPI_COUNT == 2, "Should have 2 SPI");
static_assert(Traits::Peripherals::HAS_USB == true, "Should have USB");
static_assert(Traits::Peripherals::HAS_CAN == true, "Should have CAN");

// Test that validation concept works
template<uint8_t Pin>
requires pins::ValidPin<Pin>
constexpr uint8_t get_pin() {
    return Pin;
}

// This should compile
constexpr uint8_t valid_pin = get_pin<pins::PC13>();

// Uncomment to test compile error:
// constexpr uint8_t invalid_pin = get_pin<100>();  // Should fail to compile

int main() {
    // All tests are compile-time, so if this compiles, tests pass
    return 0;
}
