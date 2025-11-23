/**
 * @file test_gpio_mock.cpp
 * @brief Example of host-based GPIO testing using mock registers
 *
 * This example demonstrates how to test GPIO code on the host (x86/ARM)
 * without requiring physical embedded hardware. The mock GPIO implementation
 * provides the same API as embedded platforms but uses in-memory registers.
 *
 * Benefits:
 * - Fast test execution (no hardware upload)
 * - Deterministic behavior
 * - Easy CI/CD integration
 * - Debuggable with standard tools (gdb, lldb)
 * - Same code works on embedded and host
 *
 * Build:
 *   g++ -std=c++20 -I../../src test_gpio_mock.cpp -o test_gpio
 *   ./test_gpio
 */

#include "hal/platform/host/gpio.hpp"
#include "hal/types.hpp"

#include <cassert>
#include <iostream>

using namespace ucore::hal;
using namespace ucore::hal::host;

/**
 * @brief Test GPIO set/clear operations
 */
void test_gpio_set_clear() {
    std::cout << "Test: GPIO set/clear... ";

    // Reset all mock GPIO ports to clean state
    reset_mock_gpio();

    // Define a mock LED on Port A, Pin 5
    using Led = GpioPin<GPIOA_PORT, 5>;
    Led led;

    // Configure as output
    auto result = led.setDirection(PinDirection::Output);
    assert(result.is_ok() && "setDirection should succeed");

    // Set pin HIGH
    result = led.set();
    assert(result.is_ok() && "set() should succeed");

    // Verify pin is HIGH
    auto read_result = led.read();
    assert(read_result.is_ok() && "read() should succeed");
    assert(read_result.unwrap() == true && "Pin should be HIGH");

    // Clear pin to LOW
    result = led.clear();
    assert(result.is_ok() && "clear() should succeed");

    // Verify pin is LOW
    read_result = led.read();
    assert(read_result.is_ok() && "read() should succeed");
    assert(read_result.unwrap() == false && "Pin should be LOW");

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Test GPIO toggle operation
 */
void test_gpio_toggle() {
    std::cout << "Test: GPIO toggle... ";

    reset_mock_gpio();

    using Led = GpioPin<GPIOA_PORT, 5>;
    Led led;

    led.setDirection(PinDirection::Output);

    // Initial state: LOW
    led.clear();
    assert(led.read().unwrap() == false);

    // Toggle to HIGH
    led.toggle();
    assert(led.read().unwrap() == true);

    // Toggle back to LOW
    led.toggle();
    assert(led.read().unwrap() == false);

    // Toggle again to HIGH
    led.toggle();
    assert(led.read().unwrap() == true);

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Test GPIO write operation
 */
void test_gpio_write() {
    std::cout << "Test: GPIO write... ";

    reset_mock_gpio();

    using Led = GpioPin<GPIOA_PORT, 5>;
    Led led;

    led.setDirection(PinDirection::Output);

    // Write HIGH
    led.write(true);
    assert(led.read().unwrap() == true);

    // Write LOW
    led.write(false);
    assert(led.read().unwrap() == false);

    // Write HIGH again
    led.write(true);
    assert(led.read().unwrap() == true);

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Test GPIO direction configuration
 */
void test_gpio_direction() {
    std::cout << "Test: GPIO direction... ";

    reset_mock_gpio();

    using Pin = GpioPin<GPIOA_PORT, 10>;
    Pin pin;

    // Configure as output
    pin.setDirection(PinDirection::Output);
    assert(pin.isOutput().unwrap() == true && "Should be output");

    // Configure as input
    pin.setDirection(PinDirection::Input);
    assert(pin.isOutput().unwrap() == false && "Should be input");

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Test GPIO pull resistor configuration
 */
void test_gpio_pull() {
    std::cout << "Test: GPIO pull resistors... ";

    reset_mock_gpio();

    using Pin = GpioPin<GPIOB_PORT, 3>;
    Pin pin;

    // Test pull-up
    auto result = pin.setPull(PinPull::PullUp);
    assert(result.is_ok());

    // Verify register state
    auto* regs = pin.get_mock_registers();
    uint32_t pupdr = regs->PUPDR.load();
    uint32_t expected_pullup = 0x1 << (3 * 2);  // Pull-up = 01, pin 3 → bits 6-7
    assert((pupdr & (0x3 << 6)) == expected_pullup && "PUPDR should reflect pull-up");

    // Test pull-down
    result = pin.setPull(PinPull::PullDown);
    assert(result.is_ok());

    pupdr = regs->PUPDR.load();
    uint32_t expected_pulldown = 0x2 << (3 * 2);  // Pull-down = 10
    assert((pupdr & (0x3 << 6)) == expected_pulldown && "PUPDR should reflect pull-down");

    // Test no pull
    result = pin.setPull(PinPull::None);
    assert(result.is_ok());

    pupdr = regs->PUPDR.load();
    assert((pupdr & (0x3 << 6)) == 0 && "PUPDR should reflect no pull");

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Test GPIO drive mode configuration
 */
void test_gpio_drive() {
    std::cout << "Test: GPIO drive mode... ";

    reset_mock_gpio();

    using Pin = GpioPin<GPIOC_PORT, 7>;
    Pin pin;

    // Test push-pull (default)
    auto result = pin.setDrive(PinDrive::PushPull);
    assert(result.is_ok());

    auto* regs = pin.get_mock_registers();
    uint32_t otyper = regs->OTYPER.load();
    assert((otyper & (1 << 7)) == 0 && "Push-pull → bit should be 0");

    // Test open-drain
    result = pin.setDrive(PinDrive::OpenDrain);
    assert(result.is_ok());

    otyper = regs->OTYPER.load();
    assert((otyper & (1 << 7)) != 0 && "Open-drain → bit should be 1");

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Test multiple pins on same port don't interfere
 */
void test_multiple_pins() {
    std::cout << "Test: Multiple pins on same port... ";

    reset_mock_gpio();

    using Pin0 = GpioPin<GPIOA_PORT, 0>;
    using Pin1 = GpioPin<GPIOA_PORT, 1>;
    using Pin2 = GpioPin<GPIOA_PORT, 2>;

    Pin0 pin0;
    Pin1 pin1;
    Pin2 pin2;

    // Configure all as output
    pin0.setDirection(PinDirection::Output);
    pin1.setDirection(PinDirection::Output);
    pin2.setDirection(PinDirection::Output);

    // Set different states
    pin0.set();
    pin1.clear();
    pin2.set();

    // Verify states are independent
    assert(pin0.read().unwrap() == true);
    assert(pin1.read().unwrap() == false);
    assert(pin2.read().unwrap() == true);

    // Toggle pin1 should not affect others
    pin1.toggle();

    assert(pin0.read().unwrap() == true);
    assert(pin1.read().unwrap() == true);
    assert(pin2.read().unwrap() == true);

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Test register state inspection (for debugging)
 */
void test_register_inspection() {
    std::cout << "Test: Register inspection... ";

    reset_mock_gpio();

    using Led = GpioPin<GPIOA_PORT, 5>;
    Led led;

    // Configure pin
    led.setDirection(PinDirection::Output);
    led.setDrive(PinDrive::PushPull);
    led.setPull(PinPull::PullUp);
    led.set();

    // Inspect register state
    auto* regs = led.get_mock_registers();

    // Verify MODER (output = 01 for pin 5 → bits 10-11)
    uint32_t moder = regs->MODER.load();
    assert(((moder >> 10) & 0x3) == 0x1 && "MODER should show output");

    // Verify OTYPER (push-pull = 0 for pin 5)
    uint32_t otyper = regs->OTYPER.load();
    assert(((otyper >> 5) & 0x1) == 0 && "OTYPER should show push-pull");

    // Verify PUPDR (pull-up = 01 for pin 5 → bits 10-11)
    uint32_t pupdr = regs->PUPDR.load();
    assert(((pupdr >> 10) & 0x3) == 0x1 && "PUPDR should show pull-up");

    // Verify ODR (output HIGH for pin 5)
    uint32_t odr = regs->ODR.load();
    assert(((odr >> 5) & 0x1) == 1 && "ODR should show HIGH");

    std::cout << "✓ PASSED\n";
}

/**
 * @brief Main test runner
 */
int main() {
    std::cout << "========================================\n";
    std::cout << "Host-Based GPIO Mock Testing\n";
    std::cout << "========================================\n\n";

    test_gpio_set_clear();
    test_gpio_toggle();
    test_gpio_write();
    test_gpio_direction();
    test_gpio_pull();
    test_gpio_drive();
    test_multiple_pins();
    test_register_inspection();

    std::cout << "\n========================================\n";
    std::cout << "All tests passed! ✓\n";
    std::cout << "========================================\n";

    return 0;
}
