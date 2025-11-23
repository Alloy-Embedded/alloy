# Host-Based Testing with Mock GPIO

This example demonstrates how to test MicroCore HAL code on your development machine (x86/ARM macOS/Linux) without requiring physical embedded hardware.

## Overview

The **host platform** provides mock implementations of HAL peripherals that:
- ✅ Use the **same API** as embedded platforms
- ✅ Store state in **memory** instead of hardware registers
- ✅ Allow **inspection and injection** for testing
- ✅ Compile and run on **x86/ARM** host machines
- ✅ Enable **fast** test-driven development

## Benefits

| Aspect | Embedded Testing | Host Testing |
|--------|------------------|--------------|
| **Speed** | Slow (flash upload ~10s) | Fast (instant run) |
| **Debugging** | Limited (JTAG/SWD) | Full (gdb, lldb, IDE) |
| **CI/CD** | Requires hardware | Works anywhere |
| **Iteration** | Minutes per cycle | Seconds per cycle |
| **Coverage** | Hard to measure | Standard tools work |

## Quick Start

### 1. Build and Run

```bash
# From examples/host_testing directory
g++ -std=c++20 -I../../src test_gpio_mock.cpp -o test_gpio

# Run tests
./test_gpio
```

Expected output:
```
========================================
Host-Based GPIO Mock Testing
========================================

Test: GPIO set/clear... ✓ PASSED
Test: GPIO toggle... ✓ PASSED
Test: GPIO write... ✓ PASSED
Test: GPIO direction... ✓ PASSED
Test: GPIO pull resistors... ✓ PASSED
Test: GPIO drive mode... ✓ PASSED
Test: Multiple pins on same port... ✓ PASSED
Test: Register inspection... ✓ PASSED

========================================
All tests passed! ✓
========================================
```

### 2. Using with Google Test

```cpp
#include <gtest/gtest.h>
#include "hal/platform/host/gpio.hpp"

using namespace ucore::hal::host;

class GpioTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset mock GPIO before each test
        reset_mock_gpio();
    }
};

TEST_F(GpioTest, SetClearPin) {
    using Led = GpioPin<GPIOA_PORT, 5>;
    Led led;

    led.setDirection(PinDirection::Output);
    led.set();

    EXPECT_TRUE(led.read().value());

    led.clear();
    EXPECT_FALSE(led.read().value());
}

TEST_F(GpioTest, TogglePin) {
    using Led = GpioPin<GPIOA_PORT, 5>;
    Led led;

    led.setDirection(PinDirection::Output);
    led.clear();

    led.toggle();
    EXPECT_TRUE(led.read().value());

    led.toggle();
    EXPECT_FALSE(led.read().value());
}
```

## API Compatibility

The host GPIO provides **100% API compatibility** with embedded platforms:

```cpp
// This code works IDENTICALLY on host and embedded!
template<typename GpioPin>
void blink_led(GpioPin& led) {
    led.setDirection(PinDirection::Output);

    while (true) {
        led.toggle();
        delay_ms(500);
    }
}

// On embedded (STM32F4):
using EmbeddedLed = ucore::hal::stm32f4::GpioPin<GPIOA_BASE, 5>;

// On host (testing):
using HostLed = ucore::hal::host::GpioPin<GPIOA_PORT, 5>;
```

## Register Inspection

The mock GPIO allows you to **inspect internal register state** for testing:

```cpp
using Led = GpioPin<GPIOA_PORT, 5>;
Led led;

led.setDirection(PinDirection::Output);
led.setPull(PinPull::PullUp);
led.set();

// Inspect register state
auto* regs = led.get_mock_registers();

uint32_t moder = regs->MODER.load();
uint32_t pupdr = regs->PUPDR.load();
uint32_t odr = regs->ODR.load();

// Verify configuration
assert(((moder >> 10) & 0x3) == 0x1);  // Output mode
assert(((pupdr >> 10) & 0x3) == 0x1);  // Pull-up
assert(((odr >> 5) & 0x1) == 1);        // HIGH
```

## Input Injection

You can **inject input values** to simulate external signals:

```cpp
using Button = GpioPin<GPIOC_PORT, 13>;
Button btn;

btn.setDirection(PinDirection::Input);
btn.setPull(PinPull::PullUp);

// Simulate button press (active low)
btn.inject_input(false);

// Read input (will be LOW)
assert(btn.read().value() == false);

// Simulate button release
btn.inject_input(true);

assert(btn.read().value() == true);
```

## Thread Safety

Mock registers use `std::atomic<uint32_t>` for thread-safe access:

```cpp
// Safe to use from multiple threads
std::thread t1([&led]() { led.set(); });
std::thread t2([&led]() { led.clear(); });

t1.join();
t2.join();
```

## Architecture

```
┌─────────────────────────────────────┐
│   Application Code (Hardware Agnostic) │
└─────────────────┬───────────────────┘
                  │
        ┌─────────┴─────────┐
        │                   │
┌───────▼──────┐   ┌────────▼────────┐
│  Embedded    │   │   Host/Mock     │
│  Platform    │   │   Platform      │
│              │   │                 │
│ • STM32F4    │   │ • In-memory     │
│ • SAME70     │   │   registers     │
│ • Real HW    │   │ • Inspectable   │
│   registers  │   │ • Fast tests    │
└──────────────┘   └─────────────────┘
```

## Best Practices

### 1. Write Platform-Agnostic Code

```cpp
// ✓ Good - works everywhere
template<typename GpioPin>
class LedController {
    GpioPin led_;
public:
    void on() { led_.set(); }
    void off() { led_.clear(); }
};

// ✗ Bad - platform-specific
void turn_on_led() {
    *(volatile uint32_t*)0x40020014 |= (1 << 5);  // Hardcoded!
}
```

### 2. Use Reset in Tests

```cpp
TEST(GpioTest, MyTest) {
    reset_mock_gpio();  // ✓ Clean state for each test

    // Your test code...
}
```

### 3. Test Both Happy and Error Paths

```cpp
TEST(GpioTest, ConfigurationErrors) {
    using Pin = GpioPin<GPIOA_PORT, 5>;
    Pin pin;

    // Test valid operations
    EXPECT_TRUE(pin.setDirection(PinDirection::Output).is_ok());

    // Test edge cases
    // (Mock always succeeds, but real hardware might fail)
}
```

## Limitations

The host platform mock has some limitations compared to real hardware:

| Feature | Embedded | Host Mock |
|---------|----------|-----------|
| **Timing** | Real timing | No delays |
| **Interrupts** | Hardware IRQs | Not implemented |
| **DMA** | Real DMA | Not implemented |
| **Analog** | Real ADC/DAC | Not implemented |
| **Power states** | Low power modes | Not simulated |

## Next Steps

- [ ] Add mock UART implementation
- [ ] Add mock SPI implementation
- [ ] Add mock I2C implementation
- [ ] Add interrupt simulation
- [ ] Add timing/delay simulation

## Related Documentation

- [Porting Guide](../../docs/porting_platform.md) - Adding new platforms
- [Testing Strategy](../../docs/TESTING.md) - Overall test approach
- [GPIO Concepts](../../docs/concepts.md#gpiopin) - GPIO concept definition

---

**Note**: The host platform is for **testing only**. Always validate on real hardware before deployment!
