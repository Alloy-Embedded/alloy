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

### GPIO Examples

#### Simple Tier (Beginner-Friendly)

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

#### Fluent Tier (Production-Ready)

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

#### Expert Tier (Performance-Critical)

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

### UART Examples

#### Simple Tier (Beginner-Friendly)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `simple_uart_echo.cpp` | Echo server | `Uart::quick_setup<>()`, `write_str()`, `read()` |

**API Style**:
```cpp
// One-liner UART setup
auto uart = Usart2::quick_setup<TxPin, RxPin>(BaudRate{115200});
uart.initialize();

// Simple I/O
uart.write_str("Hello!\r\n");
uint8_t buffer[64];
uart.read(buffer, sizeof(buffer));
```

**When to use**:
- ✅ Serial communication debugging
- ✅ Simple data logging
- ✅ Terminal interfaces
- ✅ Learning UART basics

#### Fluent Tier (Production-Ready)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `fluent_uart_config.cpp` | Sensor data transmission | Custom baud rate, parity, builder pattern |

**API Style**:
```cpp
// Builder pattern with custom configuration
auto uart = Usart2Builder()
    .with_tx_pin<TxPin>()
    .with_rx_pin<RxPin>()
    .baudrate(BaudRate{9600})
    .parity(UartParity::EVEN)     // Error detection
    .data_bits(8)
    .stop_bits(1)
    .initialize();

// Result-based error handling
auto result = uart.write_str("Sensor data");
if (result.is_err()) { /* handle error */ }
```

**When to use**:
- ✅ Custom serial protocols
- ✅ Error detection with parity
- ✅ Non-standard baud rates
- ✅ Production sensor interfaces

#### Expert Tier (Performance-Critical)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `expert_uart_lowpower.cpp` | Low-power LPUART (STM32G0) | Wakeup from STOP mode, FIFO, compile-time validation |

**API Style**:
```cpp
// Compile-time validated configuration
constexpr auto config = Lpuart1ExpertConfig::create_config(
    PeripheralId::LPUART1,
    PinId::PA2, PinId::PA3,
    9600, 8, 1, 0
);
static_assert(config.is_valid(), config.error_message());

// Platform-specific low-power features (STM32G0)
auto lpuart = Lpuart1::quick_setup<TxPin, RxPin>(BaudRate{9600});
lpuart.initialize();
lpuart.enable_wakeup();  // Wake from STOP mode on RX
lpuart.enable_fifo();    // 8-byte FIFO

// Ultra-low power operation
enter_stop_mode();  // ~1-2 µA vs ~10 mA in RUN mode
```

**When to use**:
- ✅ Battery-powered applications
- ✅ Ultra-low power requirements
- ✅ Wakeup from deep sleep
- ✅ Platform-specific features (LPUART, FIFO)

**Power Comparison**:
- Simple/Fluent (continuous RUN): ~10 mA
- Expert (STOP mode + LPUART): ~1-2 µA
- **Power savings: 5000x lower consumption**

### SPI Examples

#### Simple Tier (Beginner-Friendly)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `simple_spi_master.cpp` | SPI master communication | `Spi::quick_setup<>()`, `transfer_byte()`, `transmit()` |

**API Style**:
```cpp
// One-liner SPI setup (Mode0, 1 MHz default)
auto spi = Spi1::quick_setup<MosiPin, MisoPin, SckPin>(1000000);
spi.initialize();

// Simple transfers
spi.transmit_byte(0x55);           // Send byte
uint8_t data = spi.receive_byte(); // Read byte
spi.transfer_byte(tx_data);        // Full-duplex
```

**When to use**:
- ✅ Simple SPI devices (sensors, accelerometers)
- ✅ Learning SPI protocol
- ✅ Quick prototyping
- ✅ Basic sensor reading

#### Fluent Tier (Production-Ready)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `fluent_spi_display.cpp` | LCD display driver | Custom SPI mode, higher speed, TX-only, builder pattern |

**API Style**:
```cpp
// Builder pattern with custom SPI configuration
auto spi = Spi1Builder()
    .with_pins<MosiPin, MisoPin, SckPin>()
    .clock_speed(8000000)              // 8 MHz for fast updates
    .mode(SpiMode::Mode3)              // CPOL=1, CPHA=1 (display-specific)
    .bit_order(SpiBitOrder::MsbFirst)
    .initialize();

// Bulk transfers for performance
uint8_t pixel_buffer[256];
spi.transmit(pixel_buffer, sizeof(pixel_buffer));
```

**When to use**:
- ✅ LCD/OLED displays
- ✅ Custom SPI modes (Mode1-3)
- ✅ Higher clock speeds (8-16 MHz)
- ✅ Write-heavy operations

**SPI Modes**:
- Mode0 (CPOL=0, CPHA=0): Most common (SD cards, sensors)
- Mode1 (CPOL=0, CPHA=1): Some sensors
- Mode2 (CPOL=1, CPHA=0): Rare
- Mode3 (CPOL=1, CPHA=1): Common for displays (ST7735, ILI9341)

#### Expert Tier (Performance-Critical)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `expert_spi_dma.cpp` | High-throughput DMA transfers | Compile-time config, DMA, zero-copy buffers |

**API Style**:
```cpp
// Compile-time configuration (zero runtime overhead)
constexpr auto spi_config = Spi1Expert::configure(
    SpiMode::Mode0,
    16000000,        // 16 MHz max speed
    SpiBitOrder::MsbFirst,
    SpiDataSize::Bits8,
    PinId::PA7, PinId::PA6, PinId::PA5
);

// Large buffer transfers with DMA
alignas(4) uint8_t tx_buffer[512];
alignas(4) uint8_t rx_buffer[512];
expert::spi::transfer(spi_config, tx_buffer, rx_buffer, 512);
```

**When to use**:
- ✅ High-throughput applications (>1 MB/sec)
- ✅ Large data transfers (>256 bytes)
- ✅ DMA-driven streaming
- ✅ Real-time data acquisition

**Performance Comparison**:
- Simple/Fluent (polling @ 1 MHz): ~125 KB/sec, 100% CPU usage
- Expert (DMA @ 16 MHz): ~2 MB/sec, <5% CPU usage
- **Throughput: 16x faster, CPU usage: 95% lower**

### I2C Examples

#### Simple Tier (Beginner-Friendly)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `simple_i2c_sensor.cpp` | I2C sensor reading | `I2c::quick_setup<>()`, device scanning, register access |

**API Style**:
```cpp
// One-liner I2C setup (100 kHz Standard-mode)
auto i2c = I2c1::quick_setup<SdaPin, SclPin>(I2cSpeed::Standard_100kHz);
i2c.initialize();

// Write-then-read pattern
uint8_t reg = 0x75;  // WHO_AM_I register
i2c.write(0x68, &reg, 1);
uint8_t device_id;
i2c.read(0x68, &device_id, 1);
```

**When to use**:
- ✅ Simple I2C sensors (accelerometer, temperature, RTC)
- ✅ EEPROM reading/writing
- ✅ Learning I2C protocol
- ✅ Device discovery and testing

#### Fluent Tier (Production-Ready)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `fluent_i2c_multi_sensor.cpp` | Multi-sensor data fusion | Builder pattern, 400 kHz, sensor drivers, error handling |

**API Style**:
```cpp
// Builder pattern with custom I2C configuration
auto i2c = I2c1Builder()
    .with_pins<SdaPin, SclPin>()
    .speed(I2cSpeed::Fast_400kHz)          // 400 kHz for faster sensors
    .addressing_mode(I2cAddressing::SevenBit)
    .enable_clock_stretching()
    .initialize();

// Result-based error handling
auto result = i2c.write(0x68, data, length);
if (result.is_err()) { /* handle error */ }
```

**When to use**:
- ✅ Multiple I2C sensors on same bus
- ✅ Higher speeds (400 kHz Fast-mode)
- ✅ Sensor data fusion (accel + gyro + mag)
- ✅ Production sensor interfaces

**I2C Speeds**:
- 100 kHz (Standard-mode): Most common, widest compatibility
- 400 kHz (Fast-mode): Modern sensors, faster data acquisition
- 1 MHz (Fast-mode Plus): High-speed sensors (STM32F7/G0 only)

#### Expert Tier (Performance-Critical)

| Example | Description | Demonstrates |
|---------|-------------|--------------|
| `expert_i2c_eeprom_dma.cpp` | EEPROM with error recovery | Compile-time config, page writes, wear leveling, bus recovery |

**API Style**:
```cpp
// Compile-time configuration (zero runtime overhead)
constexpr auto i2c_config = I2c1Expert::configure(
    I2cSpeed::Fast_400kHz,
    I2cAddressing::SevenBit,
    PinId::PB9,  // SDA
    PinId::PB8   // SCL
);

expert::i2c::initialize(i2c_config);

// Page write to EEPROM (32 bytes at once)
uint8_t data[32] = {...};
eeprom.write_page(0x0000, data, 32);
```

**When to use**:
- ✅ EEPROM/Flash memory access
- ✅ Critical data storage
- ✅ Error recovery (bus lockup)
- ✅ Wear leveling for longevity

**Performance Comparison**:
- Byte-by-byte write: 32 bytes × 5ms = 160ms
- Page write (Expert): 1 page × 5ms = 5ms
- **Speedup: 32x faster**

**Error Recovery**:
- Bus lockup detection
- GPIO-based recovery (9 clock pulses)
- Automatic STOP condition
- Robust for industrial environments

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

# Build GPIO examples
cmake --build build --target simple_gpio_blink fluent_gpio_blink expert_gpio_blink

# Build UART examples
cmake --build build --target simple_uart_echo fluent_uart_config expert_uart_lowpower
```

### Build Single Example

```bash
# GPIO example
cmake --build build --target simple_gpio_blink

# UART example
cmake --build build --target simple_uart_echo
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
