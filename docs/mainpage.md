# MicroCore Framework {#mainpage}

Welcome to the **MicroCore** API documentation. MicroCore is a modern C++20/23 embedded systems framework designed for bare-metal development with zero-overhead abstractions.

## Quick Links

- @ref getting_started "Getting Started Tutorial"
- @ref adding_board "Adding a New Board"
- @ref porting_platform "Porting to a New Platform"
- @ref concepts "C++20 Concepts Reference"
- @ref examples "Examples"

## Architecture Overview

MicroCore uses a **5-layer architecture** that provides portability without sacrificing performance:

```
┌─────────────────────────────────────┐
│     Application Layer               │  Your code
├─────────────────────────────────────┤
│     Board Abstraction               │  board::led::on()
├─────────────────────────────────────┤
│     Platform API Layer              │  Gpio<Pin>, Uart<Config>
├─────────────────────────────────────┤
│     Hardware Policy Layer (CRTP)    │  GpioHardwarePolicy<Port, Pin>
├─────────────────────────────────────┤
│     Generated Registers             │  GPIO_TypeDef, RCC, etc.
└─────────────────────────────────────┘
```

## Key Features

### 🚀 Zero-Overhead Abstractions

All abstractions compile down to **direct register access** with no runtime cost:

```cpp
// High-level API
board::led::toggle();

// Compiles to single instruction
GPIOA->ODR ^= (1 << 5);  // ARM: STR instruction
```

### 🎯 Type-Safe C++20 Concepts

Compile-time interfaces ensure correctness:

```cpp
template <GpioPin Pin>
void blink() {
    Pin::configure_output();
    Pin::set_high();
}
```

### 🔧 Hardware Policy Pattern (CRTP)

Separates what from how:

```cpp
// Platform-independent
template <typename Policy>
class Gpio {
    void set_high() { Policy::set_high(); }
};

// Platform-specific (STM32)
template <uint32_t Port, uint8_t Pin>
struct GpioHardwarePolicy {
    static void set_high() {
        GPIO[Port]->BSRR = (1 << Pin);
    }
};
```

### 📦 Board Abstraction

Write portable code:

```cpp
#include "board.hpp"

int main() {
    board::init();

    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

### 🔄 YAML-Based Board Configuration

Declarative hardware configuration:

```yaml
leds:
  - name: led_green
    port: GPIOA
    pin: 5
    active_high: true

clock:
  source: PLL
  system_clock_hz: 84000000
```

## Supported Platforms

| Platform | Architecture | Status | Boards |
|----------|-------------|--------|--------|
| STM32F4  | Cortex-M4   | ✅ Stable | Nucleo-F401RE |
| STM32F7  | Cortex-M7   | ✅ Stable | Nucleo-F722ZE |
| STM32G0  | Cortex-M0+  | ✅ Stable | Nucleo-G071RB, Nucleo-G0B1RE |
| SAME70   | Cortex-M7   | ✅ Stable | SAME70 Xplained Ultra |
| Host     | x86_64      | ✅ Testing | Linux, macOS |

## Getting Started

### Installation

```bash
git clone https://github.com/lgili/corezero.git
cd corezero
```

### Build Your First Example

```bash
# List available boards
./ucore list boards

# Build blink example
./ucore build nucleo_f401re blink

# Flash to board
./ucore flash nucleo_f401re blink
```

### Create a New Project

```bash
# Create new board configuration
./ucore new-board my_custom_board

# Generate C++ code from YAML
./ucore generate-board my_custom_board

# Build your code
./ucore build my_custom_board my_app
```

## Core Concepts

### C++20 Concepts

MicroCore uses C++20 concepts for compile-time interface checking:

- @ref GpioPin - GPIO pin interface
- @ref SystemClock - System clock interface
- @ref UartPeripheral - UART peripheral interface
- @ref SpiPeripheral - SPI peripheral interface
- @ref I2cPeripheral - I2C peripheral interface

### Hardware Policies

Hardware policies implement platform-specific register access:

- @ref ucore::hal::st::stm32f4::GpioHardwarePolicy - STM32F4 GPIO
- @ref ucore::hal::st::stm32f7::GpioHardwarePolicy - STM32F7 GPIO
- @ref ucore::hal::same70::GpioHardwarePolicy - SAME70 PIO
- @ref ucore::hal::host::HostGpioHardwarePolicy - Host platform (testing)

### Platform APIs

High-level platform APIs built on hardware policies:

- @ref ucore::hal::Gpio - GPIO abstraction
- @ref ucore::hal::Uart - UART abstraction
- @ref ucore::hal::Spi - SPI abstraction
- @ref ucore::hal::I2c - I2C abstraction

## Examples

### Blink LED

```cpp
#include "board.hpp"

int main() {
    board::init();

    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }
}
```

### UART Echo

```cpp
#include "board.hpp"
#include "hal/api/uart.hpp"

int main() {
    board::init();

    auto& uart = board::uart::console;
    uart.write_string("Hello, MicroCore!\r\n");

    while (true) {
        if (uart.available()) {
            char c = uart.read_byte();
            uart.write_byte(c);
        }
    }
}
```

### Direct GPIO Control

```cpp
#include "hal/gpio.hpp"

using Led = Gpio<GpioPin<GPIOA, 5>>;

int main() {
    Led::configure_output();

    while (true) {
        Led::set_high();
        delay_ms(500);
        Led::set_low();
        delay_ms(500);
    }
}
```

## Design Philosophy

### Three Abstraction Tiers

MicroCore provides three levels of abstraction:

1. **Simple Tier** - Board-level API for beginners
   ```cpp
   board::led::on();
   ```

2. **Fluent Tier** - Method chaining for readability
   ```cpp
   Led.configure().output().set_high();
   ```

3. **Expert Tier** - Direct policy access for maximum control
   ```cpp
   GpioHardwarePolicy<GPIOA, 5>::set_high();
   ```

### Zero Runtime Cost

All abstractions are **fully inlined** and compile to **direct register access**:

```cpp
// C++ code
Gpio<Pin<GPIOA, 5>>::toggle();

// Assembly (ARM)
ldr  r0, =0x40020014    ; ODR address
ldr  r1, [r0]           ; Read current value
eor  r1, r1, #32        ; Toggle bit 5
str  r1, [r0]           ; Write back
```

### Compile-Time Configuration

Everything is resolved at **compile time**:

- Pin mappings
- Clock frequencies
- Peripheral addresses
- Feature flags

No runtime overhead, no dynamic dispatch, no virtual functions.

## Testing

### Host Platform Testing

Test your logic without hardware:

```cpp
#include "hal/vendors/host/gpio_hardware_policy.hpp"

TEST_CASE("LED blinks correctly") {
    using Led = HostGpioHardwarePolicy<0, 5>;

    Led::configure_output();
    Led::set_high();

    auto& regs = Led::get_registers_for_testing();
    REQUIRE((regs.ODR.load() & (1u << 5)) != 0);
}
```

See @ref host_testing for details.

## Contributing

See our [Contributing Guide](CONTRIBUTING.md) for:
- Code style guidelines
- Pull request process
- Testing requirements
- Documentation standards

## API Stability

MicroCore follows semantic versioning:

- **Major** version: Breaking API changes
- **Minor** version: New features, backward compatible
- **Patch** version: Bug fixes

Current version: **0.1.0** (Pre-release)

## License

MicroCore is released under the MIT License. See [LICENSE](../LICENSE) for details.

## Support

- **Documentation**: https://lgili.github.io/corezero/
- **Issues**: https://github.com/lgili/corezero/issues
- **Discussions**: https://github.com/lgili/corezero/discussions

---

**Next Steps:**
- @ref getting_started - Build your first application
- @ref concepts - Learn about C++20 concepts
- @ref examples - Explore example projects
