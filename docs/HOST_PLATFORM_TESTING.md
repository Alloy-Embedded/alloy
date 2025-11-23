# Host Platform Testing Guide

The **host platform** enables unit testing of MicroCore applications without requiring physical hardware. It provides mock implementations of hardware peripherals that follow the same API as embedded targets.

## Why Host Platform Testing?

✅ **Fast Development**: Test logic without flashing hardware
✅ **CI/CD Integration**: Automated testing in GitHub Actions
✅ **Debugging**: Use standard debuggers (gdb, lldb, VS Code)
✅ **Coverage**: Run coverage tools on host builds
✅ **Portability**: Validates code works across platforms

## Architecture

The host platform follows the **same hardware policy pattern** as embedded targets:

```
Application Code
    ↓
Platform API (Gpio, Uart, etc.)
    ↓
Hardware Policy (HostGpioHardwarePolicy)
    ↓
Mock Registers (std::atomic<uint32_t>)
```

This ensures that:
- Application code is **identical** for host and embedded
- Tests validate **real behavior**, not mocked interfaces
- Platform abstraction is **truly portable**

## Quick Start

### 1. Write Application Code

```cpp
// examples/blink/main.cpp
#include "board.hpp"

int main() {
    board::init();

    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

### 2. Build for Host

```bash
./ucore build host blink
./build-host/examples/blink/blink
```

**Output:**
```
[GPIO Mock] Port A Pin 5 configured as OUTPUT
[GPIO Mock] Port A Pin 5 toggled to HIGH
[GPIO Mock] Port A Pin 5 toggled to LOW
...
```

### 3. Write Unit Tests

```cpp
// tests/host/test_my_application.cpp
#include <catch2/catch_test_macros.hpp>
#include "hal/vendors/host/gpio_hardware_policy.hpp"

using namespace ucore::hal::host;

TEST_CASE("LED blinks correctly") {
    using LedPin = HostGpioHardwarePolicy<0, 5>;

    LedPin::configure_output();
    LedPin::set_high();

    auto& regs = LedPin::get_registers_for_testing();
    REQUIRE((regs.ODR.load() & (1u << 5)) != 0);
}
```

## Hardware Policy API

### GPIO Hardware Policy

The `HostGpioHardwarePolicy` provides the same API as embedded platforms:

```cpp
#include "hal/vendors/host/gpio_hardware_policy.hpp"

using namespace ucore::hal::host;

// Template parameters: <Port, Pin>
using LedPin = HostGpioHardwarePolicy<0, 5>;  // Port A, Pin 5

// Configuration
LedPin::configure_output();
LedPin::configure_input();
LedPin::configure_pull_up();
LedPin::configure_pull_down();

// Operations
LedPin::set_high();
LedPin::set_low();
LedPin::toggle();
bool state = LedPin::read();

// Testing utilities
auto& regs = LedPin::get_registers_for_testing();
LedPin::reset_registers();
LedPin::set_debug_output(false);  // Silence console output
```

### Mock Register Structure

The host platform simulates real hardware registers:

```cpp
struct MockGpioRegisters {
    std::atomic<uint32_t> MODER;     // Mode register
    std::atomic<uint32_t> OTYPER;    // Output type
    std::atomic<uint32_t> OSPEEDR;   // Output speed
    std::atomic<uint32_t> PUPDR;     // Pull-up/down
    std::atomic<uint32_t> IDR;       // Input data
    std::atomic<uint32_t> ODR;       // Output data
    std::atomic<uint32_t> BSRR;      // Bit set/reset
    std::atomic<uint32_t> LCKR;      // Lock register
    std::atomic<uint32_t> AFRL;      // Alternate function low
    std::atomic<uint32_t> AFRH;      // Alternate function high
};
```

## Testing Patterns

### Pattern 1: Register State Verification

Verify that operations update registers correctly:

```cpp
TEST_CASE("GPIO operations update registers") {
    using Pin = HostGpioHardwarePolicy<0, 5>;

    Pin::configure_output();
    auto& regs = Pin::get_registers_for_testing();

    // Verify MODER was set to output (01)
    uint32_t moder = regs.MODER.load();
    uint32_t pin_mode = (moder >> (5 * 2)) & 0b11;
    REQUIRE(pin_mode == 0b01);

    // Verify set_high updates ODR
    Pin::set_high();
    REQUIRE((regs.ODR.load() & (1u << 5)) != 0);
}
```

### Pattern 2: Behavior Testing

Test application logic without register details:

```cpp
TEST_CASE("LED toggle alternates state") {
    using Led = HostGpioHardwarePolicy<0, 5>;

    Led::configure_output();
    Led::set_low();

    // Toggle should flip state
    Led::toggle();
    REQUIRE(Led::read() == true);

    Led::toggle();
    REQUIRE(Led::read() == false);
}
```

### Pattern 3: Multiple Pin Interaction

Test complex scenarios with multiple pins:

```cpp
TEST_CASE("Multiple LEDs can be controlled independently") {
    using Led1 = HostGpioHardwarePolicy<0, 0>;
    using Led2 = HostGpioHardwarePolicy<0, 1>;
    using Led3 = HostGpioHardwarePolicy<0, 2>;

    Led1::configure_output();
    Led2::configure_output();
    Led3::configure_output();

    // Set different states
    Led1::set_high();
    Led2::set_low();
    Led3::set_high();

    // Verify independent control
    REQUIRE(Led1::read() == true);
    REQUIRE(Led2::read() == false);
    REQUIRE(Led3::read() == true);
}
```

### Pattern 4: Input Simulation

Simulate external signals for input testing:

```cpp
TEST_CASE("Button input can be simulated") {
    using Button = HostGpioHardwarePolicy<0, 13>;

    Button::configure_input();
    Button::configure_pull_up();

    // Simulate button press by setting IDR
    auto& regs = Button::get_registers_for_testing();
    regs.IDR.store(1u << 13);  // Button pressed (HIGH)

    REQUIRE(Button::read() == true);

    regs.IDR.store(0);  // Button released (LOW)
    REQUIRE(Button::read() == false);
}
```

### Pattern 5: Test Fixtures

Use fixtures for clean test state:

```cpp
class GpioTestFixture {
public:
    GpioTestFixture() {
        // Disable debug output
        HostGpioHardwarePolicy<0, 0>::set_debug_output(false);
        // Reset all registers
        HostGpioHardwarePolicy<0, 5>::reset_registers();
    }

    ~GpioTestFixture() {
        HostGpioHardwarePolicy<0, 0>::set_debug_output(true);
    }
};

TEST_CASE("Test with fixture") {
    GpioTestFixture fixture;
    // Test code here - guaranteed clean state
}
```

## Debug Output Control

### Enable/Disable Console Output

```cpp
// Disable for automated tests
HostGpioHardwarePolicy<0, 5>::set_debug_output(false);

// Enable for debugging
HostGpioHardwarePolicy<0, 5>::set_debug_output(true);
```

### Example Debug Output

```
[GPIO Mock] Port A Pin 5 configured as OUTPUT
[GPIO Mock] Port A Pin 5 set HIGH
[GPIO Mock] Port A Pin 5 set LOW
[GPIO Mock] Port A Pin 5 toggled to HIGH
```

## Thread Safety

All host platform operations are **thread-safe**:

- Registers use `std::atomic<uint32_t>`
- Debug output protected by `std::mutex`
- Safe for concurrent test execution

```cpp
TEST_CASE("Concurrent GPIO operations are safe") {
    using Pin = HostGpioHardwarePolicy<0, 5>;
    Pin::configure_output();

    std::thread t1([]{
        for(int i = 0; i < 1000; i++) Pin::set_high();
    });

    std::thread t2([]{
        for(int i = 0; i < 1000; i++) Pin::set_low();
    });

    t1.join();
    t2.join();
    // No data races, no crashes
}
```

## CMake Integration

### Building Tests

```cmake
# tests/host/CMakeLists.txt
add_executable(test_gpio_host
    test_gpio_host_policy.cpp
)

target_link_libraries(test_gpio_host PRIVATE
    Catch2::Catch2WithMain
    ucore::hal::host
)

target_compile_definitions(test_gpio_host PRIVATE
    MICROCORE_BOARD=host
    MICROCORE_PLATFORM=linux
)
```

### Running Tests

```bash
cd build-host
ctest
```

Or with Catch2:

```bash
./tests/host/test_gpio_host --reporter console
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Host Platform Tests

on: [push, pull_request]

jobs:
  test-host:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build clang

      - name: Build Host Tests
        run: |
          cmake -B build-host -DMICROCORE_BOARD=host
          cmake --build build-host

      - name: Run Tests
        run: |
          cd build-host
          ctest --output-on-failure

      - name: Generate Coverage
        run: |
          cmake --build build-host --target coverage
```

## Comparison: Host vs Embedded

| Feature | Host Platform | Embedded Platform |
|---------|---------------|-------------------|
| **Build Target** | x86_64/ARM64 | ARM Cortex-M |
| **Registers** | Mock (`std::atomic`) | Real hardware |
| **Debugging** | gdb, lldb, VS Code | OpenOCD, J-Link |
| **Speed** | Instant | Flash + run (seconds) |
| **Coverage** | gcov/lcov | Limited |
| **API** | **Identical** | **Identical** |

## Limitations

**What host platform CAN do:**
- ✅ Test application logic
- ✅ Verify register operations
- ✅ Test state machines
- ✅ Test error handling
- ✅ Test multi-threaded code

**What host platform CANNOT do:**
- ❌ Test actual hardware timing
- ❌ Test interrupt behavior
- ❌ Test DMA operations
- ❌ Test hardware errata
- ❌ Replace hardware-in-loop testing

## Best Practices

### 1. Use Host for Unit Tests

```cpp
// Unit test (host platform)
TEST_CASE("Button debounce logic") {
    // Test logic without hardware
}
```

### 2. Use Embedded for Integration Tests

```cpp
// Integration test (on real hardware)
TEST_CASE("UART communication timing") {
    // Test with actual peripherals
}
```

### 3. Same Code, Both Platforms

```cpp
// application.cpp - works on BOTH host and embedded!
void blink_pattern() {
    board::led::on();
    board::delay_ms(100);
    board::led::off();
    board::delay_ms(900);
}
```

### 4. Test Register Details on Host

```cpp
// Verify that your abstraction uses registers correctly
TEST_CASE("Blink uses BSRR for atomic operations") {
    using Led = HostGpioHardwarePolicy<0, 5>;
    Led::configure_output();

    Led::set_high();
    // Verify BSRR was used (atomic), not ODR (read-modify-write)
    auto& regs = Led::get_registers_for_testing();
    // BSRR should have been written
}
```

## Example: Complete Test File

See `tests/host/test_gpio_host_policy.cpp` for a complete example with:
- Test fixtures
- Register verification
- Behavior testing
- Multi-pin scenarios
- Thread safety tests

## See Also

- `tests/host/test_gpio_host_policy.cpp` - Complete test examples
- `src/hal/vendors/host/gpio_hardware_policy.hpp` - Implementation
- `TESTING.md` - Overall testing strategy
- `ARCHITECTURE.md` - Hardware policy pattern explanation
