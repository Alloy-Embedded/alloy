/**
 * @file gpio_test.cpp
 * @brief Unit tests for SAME70 GPIO template implementation
 *
 * These tests verify the GPIO template behavior without requiring hardware.
 * Uses mock registers to simulate hardware behavior.
 *
 * Test coverage:
 * - Pin configuration (input/output/open-drain)
 * - Output operations (set/clear/toggle)
 * - Input operations (read)
 * - Pull resistor configuration
 * - Compile-time validation
 */

#include <catch2/catch_test_macros.hpp>
#include "gpio_mock.hpp"

// Define mock injection macros BEFORE including gpio.hpp
#define ALLOY_GPIO_MOCK_PORT() (reinterpret_cast<volatile alloy::hal::atmel::same70::atsame70q21::pioa::PIOA_Registers*>(alloy::hal::test::g_mock_gpio))

// Define test hooks for register access
#define ALLOY_GPIO_TEST_HOOK_SODR() alloy::hal::test::g_mock_gpio->sync_sodr()
#define ALLOY_GPIO_TEST_HOOK_CODR() alloy::hal::test::g_mock_gpio->sync_codr()
#define ALLOY_GPIO_TEST_HOOK_PER() alloy::hal::test::g_mock_gpio->sync_per()
#define ALLOY_GPIO_TEST_HOOK_OER() alloy::hal::test::g_mock_gpio->sync_oer()
#define ALLOY_GPIO_TEST_HOOK_ODR() alloy::hal::test::g_mock_gpio->sync_odr()
#define ALLOY_GPIO_TEST_HOOK_MDER() alloy::hal::test::g_mock_gpio->sync_mder()
#define ALLOY_GPIO_TEST_HOOK_MDDR() alloy::hal::test::g_mock_gpio->sync_mddr()
#define ALLOY_GPIO_TEST_HOOK_PUER() alloy::hal::test::g_mock_gpio->sync_puer()
#define ALLOY_GPIO_TEST_HOOK_PUDR() alloy::hal::test::g_mock_gpio->sync_pudr()
#define ALLOY_GPIO_TEST_HOOK_IFER() alloy::hal::test::g_mock_gpio->sync_ifer()
#define ALLOY_GPIO_TEST_HOOK_IFDR() alloy::hal::test::g_mock_gpio->sync_ifdr()

#include "hal/platform/same70/gpio.hpp"

using namespace alloy::hal::same70;
using namespace alloy::hal;
using namespace alloy::core;
using namespace alloy::hal::test;

// ============================================================================
// Test Fixture
// ============================================================================

/**
 * @brief Test fixture for GPIO tests
 *
 * Sets up mock environment for each test case.
 */
class GpioTestFixture {
public:
    GpioTestFixture() {
        // Mock fixture sets up global mocks
    }

    MockGpioRegisters& gpio_regs() { return mock.gpio(); }

private:
    MockGpioFixture mock;
};

// ============================================================================
// Pin Configuration Tests
// ============================================================================

TEST_CASE("GPIO setMode() configures pin as output", "[gpio][config]") {
    GpioTestFixture fixture;

    // Using pin 8 (Led0)
    auto pin = GpioPin<PIOC_BASE, 8>{};

    SECTION("Configure as output enables PIO and output") {
        auto result = pin.setMode(GpioMode::Output);

        REQUIRE(result.is_ok());
        REQUIRE(fixture.gpio_regs().is_pio_enabled(8));
        REQUIRE(fixture.gpio_regs().is_output(8));
        REQUIRE((fixture.gpio_regs().MDSR & (1u << 8)) == 0);  // Not open-drain
    }
}

TEST_CASE("GPIO setMode() configures pin as input", "[gpio][config]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};

    SECTION("Configure as input disables output") {
        auto result = pin.setMode(GpioMode::Input);

        REQUIRE(result.is_ok());
        REQUIRE(fixture.gpio_regs().is_pio_enabled(8));
        REQUIRE_FALSE(fixture.gpio_regs().is_output(8));
    }
}

TEST_CASE("GPIO setMode() configures pin as open-drain", "[gpio][config]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};

    SECTION("Configure as open-drain enables multi-driver") {
        auto result = pin.setMode(GpioMode::OutputOpenDrain);

        REQUIRE(result.is_ok());
        REQUIRE(fixture.gpio_regs().is_pio_enabled(8));
        REQUIRE(fixture.gpio_regs().is_output(8));
        REQUIRE((fixture.gpio_regs().MDSR & (1u << 8)) != 0);  // Open-drain enabled
    }
}

// ============================================================================
// Output Operation Tests
// ============================================================================

TEST_CASE("GPIO set() sets pin HIGH", "[gpio][output]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};
    pin.setMode(GpioMode::Output);

    SECTION("set() writes to SODR and updates output state") {
        auto result = pin.set();

        REQUIRE(result.is_ok());
        REQUIRE(fixture.gpio_regs().get_output_pin(8));
        REQUIRE((fixture.gpio_regs().ODSR & (1u << 8)) != 0);
    }
}

TEST_CASE("GPIO clear() sets pin LOW", "[gpio][output]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};
    pin.setMode(GpioMode::Output);

    SECTION("clear() writes to CODR and updates output state") {
        // First set it high
        pin.set();
        REQUIRE(fixture.gpio_regs().get_output_pin(8));

        // Then clear it
        auto result = pin.clear();

        REQUIRE(result.is_ok());
        REQUIRE_FALSE(fixture.gpio_regs().get_output_pin(8));
        REQUIRE((fixture.gpio_regs().ODSR & (1u << 8)) == 0);
    }
}

TEST_CASE("GPIO toggle() inverts pin state", "[gpio][output]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};
    pin.setMode(GpioMode::Output);

    SECTION("toggle() from LOW to HIGH") {
        pin.clear();  // Start LOW
        // Verify ODSR is LOW
        REQUIRE((fixture.gpio_regs().ODSR & (1u << 8)) == 0);

        auto result = pin.toggle();

        REQUIRE(result.is_ok());
        // After toggle, ODSR should be HIGH
        REQUIRE((fixture.gpio_regs().ODSR & (1u << 8)) != 0);
    }

    SECTION("toggle() from HIGH to LOW") {
        pin.set();  // Start HIGH
        // Verify ODSR is HIGH
        REQUIRE((fixture.gpio_regs().ODSR & (1u << 8)) != 0);

        auto result = pin.toggle();

        REQUIRE(result.is_ok());
        // After toggle, ODSR should be LOW
        REQUIRE((fixture.gpio_regs().ODSR & (1u << 8)) == 0);
    }

    SECTION("toggle() twice returns to original state") {
        pin.clear();  // Start LOW

        pin.toggle();  // -> HIGH
        pin.toggle();  // -> LOW

        REQUIRE_FALSE(fixture.gpio_regs().get_output_pin(8));
    }
}

TEST_CASE("GPIO write() controls pin value", "[gpio][output]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};
    pin.setMode(GpioMode::Output);

    SECTION("write(true) sets pin HIGH") {
        auto result = pin.write(true);

        REQUIRE(result.is_ok());
        REQUIRE(fixture.gpio_regs().get_output_pin(8));
    }

    SECTION("write(false) sets pin LOW") {
        pin.set();  // Start HIGH

        auto result = pin.write(false);

        REQUIRE(result.is_ok());
        REQUIRE_FALSE(fixture.gpio_regs().get_output_pin(8));
    }
}

// ============================================================================
// Input Operation Tests
// ============================================================================

TEST_CASE("GPIO read() reads pin input state", "[gpio][input]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};
    pin.setMode(GpioMode::Input);

    SECTION("read() returns HIGH when pin is HIGH") {
        fixture.gpio_regs().set_input_pin(8, true);

        auto result = pin.read();

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == true);
    }

    SECTION("read() returns LOW when pin is LOW") {
        fixture.gpio_regs().set_input_pin(8, false);

        auto result = pin.read();

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == false);
    }
}

TEST_CASE("GPIO read() reflects output state for output pins", "[gpio][output]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};
    pin.setMode(GpioMode::Output);

    SECTION("read() returns current output state") {
        pin.set();

        auto result = pin.read();

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == true);
    }
}

// ============================================================================
// Pull Resistor Tests
// ============================================================================

TEST_CASE("GPIO setPull() configures pull resistors", "[gpio][pull]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};
    pin.setMode(GpioMode::Input);

    SECTION("setPull(Up) enables pull-up") {
        auto result = pin.setPull(GpioPull::Up);

        REQUIRE(result.is_ok());
        REQUIRE((fixture.gpio_regs().PUSR & (1u << 8)) != 0);
    }

    SECTION("setPull(None) disables pull-up") {
        pin.setPull(GpioPull::Up);  // Enable first

        auto result = pin.setPull(GpioPull::None);

        REQUIRE(result.is_ok());
        REQUIRE((fixture.gpio_regs().PUSR & (1u << 8)) == 0);
    }

    SECTION("setPull(Down) returns NotSupported") {
        auto result = pin.setPull(GpioPull::Down);

        REQUIRE(result.is_error());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }
}

// ============================================================================
// Filter Tests
// ============================================================================

TEST_CASE("GPIO enableFilter() enables glitch filter", "[gpio][filter]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};

    SECTION("enableFilter() sets IFSR bit") {
        auto result = pin.enableFilter();

        REQUIRE(result.is_ok());
        REQUIRE((fixture.gpio_regs().IFSR & (1u << 8)) != 0);
    }
}

TEST_CASE("GPIO disableFilter() disables glitch filter", "[gpio][filter]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};

    SECTION("disableFilter() clears IFSR bit") {
        pin.enableFilter();  // Enable first

        auto result = pin.disableFilter();

        REQUIRE(result.is_ok());
        REQUIRE((fixture.gpio_regs().IFSR & (1u << 8)) == 0);
    }
}

// ============================================================================
// State Query Tests
// ============================================================================

TEST_CASE("GPIO isOutput() returns pin direction", "[gpio][state]") {
    GpioTestFixture fixture;

    auto pin = GpioPin<PIOC_BASE, 8>{};

    SECTION("isOutput() returns true for output pins") {
        pin.setMode(GpioMode::Output);

        auto result = pin.isOutput();

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == true);
    }

    SECTION("isOutput() returns false for input pins") {
        pin.setMode(GpioMode::Input);

        auto result = pin.isOutput();

        REQUIRE(result.is_ok());
        REQUIRE(result.value() == false);
    }
}

// ============================================================================
// Compile-Time Tests
// ============================================================================

TEST_CASE("GPIO template has correct compile-time constants", "[gpio][compile]") {
    using TestPin = GpioPin<PIOC_BASE, 8>;

    SECTION("Port base is correct") {
        STATIC_REQUIRE(TestPin::port_base == PIOC_BASE);
    }

    SECTION("Pin number is correct") {
        STATIC_REQUIRE(TestPin::pin_number == 8);
    }

    SECTION("Pin mask is computed correctly") {
        STATIC_REQUIRE(TestPin::pin_mask == (1u << 8));
        STATIC_REQUIRE(TestPin::pin_mask == 0x100);
    }
}

TEST_CASE("GPIO type aliases have correct addresses", "[gpio][compile]") {
    SECTION("Led0 uses PIOC base and pin 8") {
        STATIC_REQUIRE(Led0::port_base == PIOC_BASE);
        STATIC_REQUIRE(Led0::pin_number == 8);
    }

    SECTION("Led1 uses PIOC base and pin 9") {
        STATIC_REQUIRE(Led1::port_base == PIOC_BASE);
        STATIC_REQUIRE(Led1::pin_number == 9);
    }

    SECTION("Button0 uses PIOA base and pin 11") {
        STATIC_REQUIRE(Button0::port_base == PIOA_BASE);
        STATIC_REQUIRE(Button0::pin_number == 11);
    }
}

TEST_CASE("GPIO template validates pin number at compile time", "[gpio][compile]") {
    // This should compile fine
    using ValidPin = GpioPin<PIOC_BASE, 31>;
    STATIC_REQUIRE(ValidPin::pin_number == 31);

    // Pin 32 would fail to compile due to static_assert
    // using InvalidPin = GpioPin<PIOC_BASE, 32>;  // Won't compile!
}
