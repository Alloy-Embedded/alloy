# MicroCore API Tier Examples

This directory contains examples demonstrating MicroCore's three-tier API system implemented in `src/hal/api/`.

## System Overview

MicroCore uses the **CRTP (Curiously Recurring Template Pattern)** to provide three API tiers with zero runtime overhead:

```
Simple API  → Factory methods, one-liner setup
Fluent API  → Builder pattern, method chaining
Expert API  → Full control, platform-specific features
```

**All tiers share the same underlying implementation via CRTP** - no code duplication!

## Examples

### Simple Tier (Beginner-Friendly)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `simple_gpio_blink.cpp` | Basic LED blink | `Gpio::output<>()` factory method |
| `simple_gpio_button.cpp` | Button input + LED | `Gpio::input_pullup<>()`, active-low logic |

**API Style**:
```cpp
// One-liner factory methods
auto led = Gpio::output<led_green>();
led.on();
led.off();

auto button = Gpio::input_pullup<button_user>();
if (button.is_on().value()) { /* pressed */ }
```

**When to use**:
- ✅ Rapid prototyping
- ✅ Learning embedded development
- ✅ Simple applications
- ✅ Arduino-style projects

### Fluent Tier (Production-Ready)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `fluent_gpio_blink.cpp` | LED blink with builder | Method chaining, explicit config |

**API Style**:
```cpp
// Builder pattern with method chaining
auto led = GpioFluent<led_green>()
    .as_output()
    .push_pull()
    .active_high()
    .initial_state_off()
    .build();

led.on().unwrap();  // Result-based error handling
```

**When to use**:
- ✅ Production code
- ✅ Self-documenting configuration
- ✅ Team development
- ✅ Complex setup requirements

### Expert Tier (Performance-Critical)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `expert_gpio_blink.cpp` | High-performance blink | Full config control, platform features |

**API Style**:
```cpp
// Struct-based configuration with all parameters
GpioExpertConfig config = {
    .direction = PinDirection::Output,
    .pull = PinPull::None,
    .drive = PinDrive::PushPull,
    .active_high = true,
    .initial_state_on = false,
    .drive_strength = 3,        // Platform-specific
    .slew_rate_fast = true,     // Platform-specific
    .input_filter_enable = false,
    .filter_clock_div = 0
};

static_assert(config.is_valid(), config.error_message());
auto led = GpioExpert<led_green>(config);
```

**When to use**:
- ✅ Performance-critical code
- ✅ Platform-specific features needed
- ✅ Fine-grained hardware control
- ✅ Real-time systems

## Building Examples

### Prerequisites

```bash
# Verify toolchain
ucore doctor

# List supported boards
ucore list boards
```

### Build All Examples

```bash
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DMICROCORE_BOARD=nucleo_f401re \
  -DMICROCORE_PLATFORM=stm32f4

cmake --build build --target simple_gpio_blink fluent_gpio_blink expert_gpio_blink
```

### Build Single Example

```bash
cmake --build build --target simple_gpio_blink
```

### Flash to Board

```bash
# Using st-flash
st-flash write build/examples/api_tiers/simple_gpio_blink.bin 0x8000000

# Using OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/examples/api_tiers/simple_gpio_blink.elf verify reset exit"
```

## Tier Comparison

### Code Size Comparison

All three tiers compile to **identical assembly** with optimization:

```cpp
// Simple tier
auto led = Gpio::output<led_green>();
led.on();

// Fluent tier
auto led = GpioFluent<led_green>().as_output().build();
led.on();

// Expert tier
auto led = GpioExpert<led_green>(GpioExpertConfig::standard_output());
led.on();

// ALL compile to the same instruction:
// STR [GPIOA_BSRR], #(1 << 5)  ; Single register write
```

**Proof**: Use `objdump` to verify:
```bash
arm-none-eabi-objdump -d build/examples/api_tiers/*.elf | grep -A5 "led.on"
```

### Feature Comparison

| Feature | Simple | Fluent | Expert |
|---------|--------|--------|--------|
| **Lines of code** | 1-2 | 4-6 | 10-15 |
| **Factory methods** | ✅ | ❌ | ✅ (expert presets) |
| **Method chaining** | ❌ | ✅ | ❌ |
| **Configuration style** | Factory | Builder | Struct |
| **Active-high/low** | ✅ | ✅ | ✅ |
| **Platform features** | ❌ | ❌ | ✅ (drive strength, slew rate, filter) |
| **Compile-time validation** | ❌ | ✅ (builder state) | ✅ (static_assert) |
| **Error handling** | Result<T> | Result<T> | Result<T> |

## Architecture

### CRTP Pattern

All tiers use the same CRTP base class (`gpio_base.hpp`):

```
┌─────────────────────────────┐
│  gpio_base.hpp              │
│  GpioBase<Derived> (CRTP)   │
│                             │
│  - Shared interface methods │
│  - Zero virtual functions   │
│  - Compile-time dispatch    │
└─────────────────────────────┘
           ▲  ▲  ▲
           │  │  │
    ┌──────┘  │  └───────┐
    │         │          │
┌────────┐ ┌──────┐ ┌─────────┐
│ Simple │ │Fluent│ │ Expert  │
│  Pin   │ │Config│ │ Config  │
└────────┘ └──────┘ └─────────┘
```

**Benefits**:
- ✅ Zero runtime overhead (no vtable)
- ✅ Code reuse across tiers
- ✅ Smaller binary size
- ✅ Fixes propagate to all tiers

### File Structure

```
src/hal/api/
├── gpio_base.hpp    ← CRTP base (shared code)
├── gpio_simple.hpp  ← Simple tier (factory methods)
├── gpio_fluent.hpp  ← Fluent tier (builder pattern)
├── gpio_expert.hpp  ← Expert tier (full control)
├── uart_base.hpp    ← UART CRTP base
├── uart_simple.hpp  ← UART simple tier
├── uart_fluent.hpp  ← UART fluent tier
├── uart_expert.hpp  ← UART expert tier
└── ... (SPI, I2C, Timer, ADC, PWM)
```

## Hardware Requirements

All examples assume standard Nucleo board pinout:

| Board | LED Pin | Button Pin | Notes |
|-------|---------|------------|-------|
| nucleo_f401re | PA5 | PC13 | LD2 (green LED) |
| nucleo_f722ze | PB7 | PC13 | LD1 (green LED) |
| nucleo_g071rb | PA5 | PC13 | LD4 (green LED) |
| nucleo_g0b1re | PA5 | PC13 | LD4 (green LED) |
| same70_xplained | PC8 | PB3 | Different pinout |

**Note**: Examples use `board::pins::led_green` and `board::pins::button_user` which are board-specific.

## Troubleshooting

### Example Won't Build

1. **Check toolchain**:
   ```bash
   ucore doctor
   ```

2. **Verify board is supported**:
   ```bash
   ucore list boards
   ```

3. **Clean build**:
   ```bash
   rm -rf build
   cmake -B build ...
   ```

### Linker Errors About Missing board_config

**Problem**: `undefined reference to board::pins::led_green`

**Solution**: Ensure `board_config` library is linked:
```cmake
target_link_libraries(simple_gpio_blink PRIVATE
    ucore_hal
    board_config  # Required!
)
```

### LED Doesn't Blink

1. **Check LED pin matches your board**:
   - See `boards/<your_board>/board_config.hpp`
   - Look for `using led_green = ...`

2. **Verify clock configuration**:
   - Check `boards/<your_board>/board_config.cpp`
   - Ensure PLL is configured correctly

3. **Test with oscilloscope**:
   - Probe the LED pin
   - Verify voltage switching between 0V and 3.3V

## Migration Between Tiers

Easy to upgrade as requirements evolve:

### Simple → Fluent

```cpp
// Before (Simple)
auto led = Gpio::output<led_green>();

// After (Fluent)
auto led = GpioFluent<led_green>()
    .as_output()
    .build();
```

### Fluent → Expert

```cpp
// Before (Fluent)
auto led = GpioFluent<led_green>()
    .as_output()
    .push_pull()
    .build();

// After (Expert)
GpioExpertConfig config = {
    .direction = PinDirection::Output,
    .drive = PinDrive::PushPull,
    .drive_strength = 3  // Now we can tune drive strength!
};
auto led = GpioExpert<led_green>(config);
```

## Next Steps

1. **Learn UART APIs**: See `docs/api/uart.md` for UART tier examples
2. **Explore SPI APIs**: Check `docs/api/spi.md` for SPI configuration
3. **Read CRTP Guide**: Understand the pattern in `docs/architecture/CRTP_PATTERN.md`
4. **Study API Tiers Doc**: Complete tier documentation in `docs/API_TIERS.md`

## Related Documentation

- [API Tiers Overview](../../docs/API_TIERS.md) - Complete tier system documentation
- [CRTP Pattern Guide](../../docs/architecture/CRTP_PATTERN.md) - Deep dive into CRTP
- [GPIO API Reference](../../docs/api/gpio.md) - Full GPIO API docs
- [Board Configuration](../../docs/BOARD_CONFIGURATION.md) - Board pinout definitions

## Contributing

When adding new tier examples:

1. Follow naming: `<tier>_<peripheral>_<feature>.cpp`
2. Include comprehensive header comments
3. Document hardware requirements
4. Add to CMakeLists.txt
5. Update this README
6. Test on at least one board

## License

Copyright (c) 2024 MicroCore Project
Licensed under MIT License
