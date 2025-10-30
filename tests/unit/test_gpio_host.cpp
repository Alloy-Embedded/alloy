/// Unit tests for host GPIO implementation
///
/// Tests the mockable GPIO implementation used for development
/// on PC platforms without physical hardware.

#include "hal/host/gpio.hpp"
#include <gtest/gtest.h>

// Note: Not using "using namespace" to avoid ambiguity with GpioPin concept
// using namespace alloy::hal;
// using namespace alloy::hal::host;

// Test fixture for GPIO tests
class GpioHostTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup runs before each test
    }

    void TearDown() override {
        // Cleanup runs after each test
    }
};

/// Test: GPIO pin initial state
TEST_F(GpioHostTest, InitialState) {
    // Given: A newly created GPIO pin
    alloy::hal::host::GpioPin<13> pin;

    // When: Reading the initial state
    bool state = pin.read();

    // Then: State should be LOW (false)
    EXPECT_FALSE(state);
}

/// Test: GPIO set_high() changes state
TEST_F(GpioHostTest, SetHighChangesState) {
    // Given: A GPIO pin
    alloy::hal::host::GpioPin<13> pin;

    // When: Setting pin HIGH
    pin.set_high();

    // Then: State should be HIGH (true)
    EXPECT_TRUE(pin.read());
}

/// Test: GPIO set_low() changes state
TEST_F(GpioHostTest, SetLowChangesState) {
    // Given: A GPIO pin that is HIGH
    alloy::hal::host::GpioPin<13> pin;
    pin.set_high();

    // When: Setting pin LOW
    pin.set_low();

    // Then: State should be LOW (false)
    EXPECT_FALSE(pin.read());
}

/// Test: GPIO toggle() flips state from LOW to HIGH
TEST_F(GpioHostTest, ToggleFromLowToHigh) {
    // Given: A GPIO pin in LOW state
    alloy::hal::host::GpioPin<13> pin;
    ASSERT_FALSE(pin.read());  // Verify precondition

    // When: Toggling the pin
    pin.toggle();

    // Then: State should be HIGH
    EXPECT_TRUE(pin.read());
}

/// Test: GPIO toggle() flips state from HIGH to LOW
TEST_F(GpioHostTest, ToggleFromHighToLow) {
    // Given: A GPIO pin in HIGH state
    alloy::hal::host::GpioPin<13> pin;
    pin.set_high();
    ASSERT_TRUE(pin.read());  // Verify precondition

    // When: Toggling the pin
    pin.toggle();

    // Then: State should be LOW
    EXPECT_FALSE(pin.read());
}

/// Test: Multiple toggles alternate correctly
TEST_F(GpioHostTest, MultipleToggles) {
    // Given: A GPIO pin in LOW state
    alloy::hal::host::GpioPin<13> pin;

    // When: Toggling multiple times
    pin.toggle();  // LOW -> HIGH
    EXPECT_TRUE(pin.read());

    pin.toggle();  // HIGH -> LOW
    EXPECT_FALSE(pin.read());

    pin.toggle();  // LOW -> HIGH
    EXPECT_TRUE(pin.read());

    pin.toggle();  // HIGH -> LOW
    EXPECT_FALSE(pin.read());
}

/// Test: Pin mode can be configured
TEST_F(GpioHostTest, ConfigureMode) {
    // Given: A GPIO pin
    alloy::hal::host::GpioPin<13> pin;

    // When: Configuring as Output
    pin.configure(alloy::hal::PinMode::Output);

    // Then: Mode should be Output
    EXPECT_EQ(pin.get_mode(), alloy::hal::PinMode::Output);
}

/// Test: Different pin modes can be set
TEST_F(GpioHostTest, DifferentModes) {
    alloy::hal::host::GpioPin<13> pin;

    // Test Input mode
    pin.configure(alloy::hal::PinMode::Input);
    EXPECT_EQ(pin.get_mode(), alloy::hal::PinMode::Input);

    // Test InputPullUp mode
    pin.configure(alloy::hal::PinMode::InputPullUp);
    EXPECT_EQ(pin.get_mode(), alloy::hal::PinMode::InputPullUp);

    // Test InputPullDown mode
    pin.configure(alloy::hal::PinMode::InputPullDown);
    EXPECT_EQ(pin.get_mode(), alloy::hal::PinMode::InputPullDown);

    // Test Output mode
    pin.configure(alloy::hal::PinMode::Output);
    EXPECT_EQ(pin.get_mode(), alloy::hal::PinMode::Output);
}

/// Test: Multiple GPIO pins are independent
TEST_F(GpioHostTest, MultipleIndependentPins) {
    // Given: Multiple GPIO pins
    alloy::hal::host::GpioPin<13> pin1;
    alloy::hal::host::GpioPin<25> pin2;

    // When: Setting different states
    pin1.set_high();
    pin2.set_low();

    // Then: Each pin maintains its own state
    EXPECT_TRUE(pin1.read());
    EXPECT_FALSE(pin2.read());

    // When: Toggling one pin
    pin1.toggle();

    // Then: Only that pin's state changes
    EXPECT_FALSE(pin1.read());
    EXPECT_FALSE(pin2.read());
}

/// Test: GpioPin satisfies GpioPin concept
TEST(GpioConcept, HostPinSatisfiesConcept) {
    // Compile-time check that host::GpioPin satisfies the GpioPin concept
    static_assert(alloy::hal::GpioPin<alloy::hal::host::GpioPin<0>>,
                  "host::GpioPin must satisfy GpioPin concept");

    static_assert(alloy::hal::GpioPin<alloy::hal::host::GpioPin<25>>,
                  "host::GpioPin<25> must satisfy GpioPin concept");

    // This test always passes if it compiles
    SUCCEED();
}
