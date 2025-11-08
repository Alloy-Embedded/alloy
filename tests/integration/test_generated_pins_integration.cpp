/**
 * Integration test for generated pin headers
 * Tests that generated code integrates correctly with the build system
 */

#include <cassert>
#include <cstdio>

#include "../../src/hal/st/stm32f103/generated/stm32f103c8/pins.hpp"
#include "../../src/hal/st/stm32f103/generated/stm32f103c8/traits.hpp"

int main() {
    using namespace alloy::hal::stm32f103::stm32f103c8;

    // Test 1: Verify pin constants
    printf("Test 1: Verifying pin constants...\n");
    assert(pins::PA0 == 0);
    assert(pins::PB0 == 16);
    assert(pins::PC13 == 45);
    printf("  ✓ Pin constants correct\n");

    // Test 2: Verify pin validation
    printf("Test 2: Verifying pin validation...\n");
    constexpr bool pa0_valid = pins::is_valid_pin_v<pins::PA0>;
    constexpr bool invalid_pin = pins::is_valid_pin_v<255>;
    assert(pa0_valid == true);
    assert(invalid_pin == false);
    printf("  ✓ Pin validation works\n");

    // Test 3: Verify MCU traits
    printf("Test 3: Verifying MCU traits...\n");
    assert(Traits::FLASH_SIZE == 64 * 1024);
    assert(Traits::SRAM_SIZE == 20 * 1024);
    assert(Traits::PIN_COUNT == 37);
    printf("  ✓ MCU traits correct\n");

    // Test 4: Verify peripheral counts
    printf("Test 4: Verifying peripheral counts...\n");
    assert(Traits::Peripherals::UART_COUNT == 5);
    assert(Traits::Peripherals::I2C_COUNT == 2);
    assert(Traits::Peripherals::SPI_COUNT == 3);
    printf("  ✓ Peripheral counts correct\n");

    // Test 5: Verify feature flags
    printf("Test 5: Verifying feature flags...\n");
    assert(Traits::Peripherals::HAS_USB == true);
    assert(Traits::Peripherals::HAS_CAN == true);
    printf("  ✓ Feature flags correct\n");

    printf("\n✅ All integration tests passed!\n");
    return 0;
}
