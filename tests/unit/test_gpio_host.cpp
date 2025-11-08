/// Unit tests for host GPIO implementation
///
/// Tests the mockable GPIO implementation used for development
/// on PC platforms without physical hardware.

#include "hal/host/gpio.hpp"
#include <catch2/catch_test_macros.hpp>

// Note: Not using "using namespace" to avoid ambiguity with GpioPin concept
// using namespace alloy::hal;
// using namespace alloy::hal::host;

/// Test: GPIO pin initial state
TEST_CASE("GPIO pin initial state", "[gpio][host]") {
    // Given: A newly created GPIO pin
    alloy::hal::host::GpioPin<13> pin;

    // When: Reading the initial state
    bool state = pin.read();

    // Then: State should be LOW (false)
    REQUIRE_FALSE(state);
}

/// Test: GPIO set_high() changes state
TEST_CASE("GPIO set_high changes state", "[gpio][host]") {
    // Given: A GPIO pin
    alloy::hal::host::GpioPin<13> pin;

    // When: Setting pin HIGH
    pin.set_high();

    // Then: State should be HIGH (true)
    REQUIRE(pin.read());
}

/// Test: GPIO set_low() changes state
TEST_CASE("GPIO set_low changes state", "[gpio][host]") {
    // Given: A GPIO pin that is HIGH
    alloy::hal::host::GpioPin<13> pin;
    pin.set_high();

    // When: Setting pin LOW
    pin.set_low();

    // Then: State should be LOW (false)
    REQUIRE_FALSE(pin.read());
}

/// Test: GPIO toggle() flips state from LOW to HIGH
TEST_CASE("GPIO toggle from LOW to HIGH", "[gpio][host][toggle]") {
    // Given: A GPIO pin in LOW state
    alloy::hal::host::GpioPin<13> pin;
    REQUIRE_FALSE(pin.read());  // Verify precondition

    // When: Toggling the pin
    pin.toggle();

    // Then: State should be HIGH
    REQUIRE(pin.read());
}

/// Test: GPIO toggle() flips state from HIGH to LOW
TEST_CASE("GPIO toggle from HIGH to LOW", "[gpio][host][toggle]") {
    // Given: A GPIO pin in HIGH state
    alloy::hal::host::GpioPin<13> pin;
    pin.set_high();
    REQUIRE(pin.read());  // Verify precondition

    // When: Toggling the pin
    pin.toggle();

    // Then: State should be LOW
    REQUIRE_FALSE(pin.read());
}

/// Test: Multiple toggles alternate correctly
TEST_CASE("Multiple toggles alternate correctly", "[gpio][host][toggle]") {
    // Given: A GPIO pin in LOW state
    alloy::hal::host::GpioPin<13> pin;

    // When: Toggling multiple times
    pin.toggle();  // LOW -> HIGH
    REQUIRE(pin.read());

    pin.toggle();  // HIGH -> LOW
    REQUIRE_FALSE(pin.read());

    pin.toggle();  // LOW -> HIGH
    REQUIRE(pin.read());

    pin.toggle();  // HIGH -> LOW
    REQUIRE_FALSE(pin.read());
}

/// Test: Pin mode can be configured
TEST_CASE("Pin mode can be configured", "[gpio][host][mode]") {
    // Given: A GPIO pin
    alloy::hal::host::GpioPin<13> pin;

    // When: Configuring as Output
    pin.configure(alloy::hal::PinMode::Output);

    // Then: Mode should be Output
    REQUIRE(pin.get_mode() == alloy::hal::PinMode::Output);
}

/// Test: Different pin modes can be set
TEST_CASE("Different pin modes can be set", "[gpio][host][mode]") {
    alloy::hal::host::GpioPin<13> pin;

    SECTION("Input mode") {
        pin.configure(alloy::hal::PinMode::Input);
        REQUIRE(pin.get_mode() == alloy::hal::PinMode::Input);
    }

    SECTION("InputPullUp mode") {
        pin.configure(alloy::hal::PinMode::InputPullUp);
        REQUIRE(pin.get_mode() == alloy::hal::PinMode::InputPullUp);
    }

    SECTION("InputPullDown mode") {
        pin.configure(alloy::hal::PinMode::InputPullDown);
        REQUIRE(pin.get_mode() == alloy::hal::PinMode::InputPullDown);
    }

    SECTION("Output mode") {
        pin.configure(alloy::hal::PinMode::Output);
        REQUIRE(pin.get_mode() == alloy::hal::PinMode::Output);
    }
}

/// Test: Multiple GPIO pins are independent
TEST_CASE("Multiple GPIO pins are independent", "[gpio][host]") {
    // Given: Multiple GPIO pins
    alloy::hal::host::GpioPin<13> pin1;
    alloy::hal::host::GpioPin<25> pin2;

    SECTION("Different states on different pins") {
        // When: Setting different states
        pin1.set_high();
        pin2.set_low();

        // Then: Each pin maintains its own state
        REQUIRE(pin1.read());
        REQUIRE_FALSE(pin2.read());
    }

    SECTION("Toggle one pin doesn't affect other") {
        // Given: Initial states
        pin1.set_high();
        pin2.set_low();

        // When: Toggling one pin
        pin1.toggle();

        // Then: Only that pin's state changes
        REQUIRE_FALSE(pin1.read());
        REQUIRE_FALSE(pin2.read());
    }
}

/// Test: GpioPin satisfies GpioPin concept
TEST_CASE("Host GPIO satisfies GpioPin concept", "[gpio][host][concept]") {
    // Compile-time check that host::GpioPin satisfies the GpioPin concept
    static_assert(alloy::hal::GpioPin<alloy::hal::host::GpioPin<0>>,
                  "host::GpioPin must satisfy GpioPin concept");

    static_assert(alloy::hal::GpioPin<alloy::hal::host::GpioPin<25>>,
                  "host::GpioPin<25> must satisfy GpioPin concept");

    // This test always passes if it compiles
    REQUIRE(true);
}
