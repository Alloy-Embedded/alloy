# SPI Multi-Level API Demo

Comprehensive demonstration of the SPI multi-level API architecture.

## Overview

This example showcases all three levels of the SPI configuration API:

1. **Level 1 - Simple API**: One-liner setup with sensible defaults
2. **Level 2 - Fluent API**: Builder pattern with method chaining
3. **Level 3 - Expert API**: Full configuration control with validation

## Features Demonstrated

### Simple API (Level 1)
- Quick setup with defaults
- TX-only variant for output devices
- Compile-time pin validation
- Minimal code for common use cases

### Fluent API (Level 2)
- Self-documenting method chaining
- Incremental validation
- Preset configurations (Mode0, Mode3)
- Flexible parameter control

### Expert API (Level 3)
- Configuration as data
- Compile-time validation with detailed error messages
- Preset configurations for common scenarios
- Full control over all 17+ parameters

### DMA Integration
- Type-safe DMA channel allocation
- Compile-time conflict detection
- Automatic peripheral address setup
- Support for TX-only, RX-only, and full-duplex DMA

## Examples Included

1. **Simple API**: One-liner SPI setup
2. **Simple TX-only**: Output-only SPI for displays
3. **Fluent Builder**: Step-by-step configuration
4. **Fluent Presets**: Mode0/Mode3 presets
5. **Expert Full Config**: All parameters exposed
6. **Expert Presets**: Standard configurations
7. **Expert Validation**: Compile-time checks
8. **DMA Integration**: Type-safe DMA channels
9. **DMA Presets**: Common DMA configurations
10. **SD Card**: Real-world SD card communication
11. **SPI Flash**: High-speed flash memory access
12. **Chip Select**: RAII chip select management
13. **Byte Transfers**: Helper functions for single bytes

## Hardware Setup

### SPI0 (Full-Duplex)
- MOSI: PA7
- MISO: PA6
- SCK: PA5
- CS: PA4 (GPIO controlled)

Use case: SD cards, EEPROMs, sensors

### SPI1 (TX-Only)
- MOSI: PB15
- SCK: PB13
- CS: PB12 (GPIO controlled)

Use case: Displays, DACs, LED drivers

## Building

### For Linux (Host Testing)
```bash
mkdir build
cd build
cmake -DALLOY_PLATFORM=LINUX ..
make
./spi_multi_level_api_demo
```

### For SAME70 (Embedded Target)
```bash
mkdir build-same70
cd build-same70
cmake -DALLOY_PLATFORM=SAME70 -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/arm-none-eabi.cmake ..
make
```

## Code Examples

### Example 1: Simple API
```cpp
// One-liner setup with defaults
constexpr auto spi = Spi<PeripheralId::SPI0>::quick_setup<
    Spi0_MOSI, Spi0_MISO, Spi0_SCK
>(2000000);  // 2 MHz

static_assert(spi.is_valid(), "SPI config invalid");
```

### Example 2: Fluent API
```cpp
// Builder pattern with method chaining
auto spi = SpiBuilder<PeripheralId::SPI0>()
    .with_mosi<Spi0_MOSI>()
    .with_miso<Spi0_MISO>()
    .with_sck<Spi0_SCK>()
    .clock_speed(4000000)
    .mode(SpiMode::Mode3)
    .msb_first()
    .initialize();
```

### Example 3: Expert API
```cpp
// Full configuration control
constexpr SpiExpertConfig config = {
    .peripheral = PeripheralId::SPI0,
    .mosi_pin = PinId::PA7,
    .miso_pin = PinId::PA6,
    .sck_pin = PinId::PA5,
    .mode = SpiMode::Mode0,
    .clock_speed = 10000000,
    .bit_order = SpiBitOrder::MsbFirst,
    .data_size = SpiDataSize::Bits8,
    .enable_mosi = true,
    .enable_miso = true,
    .enable_dma_tx = false,
    .enable_dma_rx = false,
    // ... more parameters
};

static_assert(config.is_valid(), config.error_message());
```

### Example 4: DMA Integration
```cpp
// Type-safe DMA channel allocation
using Spi0TxDma = DmaConnection<
    PeripheralId::SPI0,
    DmaRequest::SPI0_TX,
    DmaStream::Stream3
>;

constexpr auto dma_config = create_spi_high_speed_dma<
    Spi0TxDma, Spi0RxDma
>(PinId::PA7, PinId::PA6, PinId::PA5, 20000000);

// Transfer with DMA
spi_dma_transfer<Spi0TxDma, Spi0RxDma>(tx_buf, rx_buf, 256);
```

## Design Principles

### Progressive Disclosure
- Level 1: Hide complexity for common cases
- Level 2: Reveal intermediate options
- Level 3: Expose all configuration

### Compile-Time Safety
- Static pin validation
- Configuration validation
- DMA conflict detection
- Type-safe error handling with Result<T, E>

### Zero Runtime Overhead
- All validation at compile-time
- Constexpr configuration
- No dynamic allocation
- No exceptions

### Self-Documenting
- Named methods describe intent
- Detailed error messages
- Preset configurations
- Comprehensive examples

## Common Use Cases

### SD Card Communication
```cpp
// Start slow for initialization
auto init_spi = Spi<PeripheralId::SPI0>::quick_setup<...>(400000);

// Speed up for data transfer
auto fast_spi = Spi<PeripheralId::SPI0>::quick_setup<...>(25000000);
```

### SPI Flash Memory
```cpp
// High-speed with DMA
auto flash = create_spi_high_speed_dma<...>(..., 50000000);

spi_dma_transmit<TxDma>(read_cmd, 4);
spi_dma_receive<RxDma>(data_buf, 4096);
```

### Display Output (TX-Only)
```cpp
// TX-only for displays
auto display = Spi<PeripheralId::SPI1>::quick_setup_master_tx<...>(8000000);

spi_device.transmit(display_data);
```

## Testing

All SPI API levels are tested in `tests/unit/test_spi_apis.cpp`:

- ✅ 5 Simple API tests
- ✅ 8 Fluent API tests
- ✅ 8 Expert API tests
- ✅ 5 DMA Integration tests

Total: 26/26 tests passing

## Documentation

For more details, see:
- `src/hal/interface/spi.hpp` - Base SPI interface
- `src/hal/spi_simple.hpp` - Level 1 API
- `src/hal/spi_fluent.hpp` - Level 2 API
- `src/hal/spi_expert.hpp` - Level 3 API
- `src/hal/spi_dma.hpp` - DMA integration
- `openspec/changes/modernize-peripheral-architecture/` - Design docs

## Related Examples

- `same70_uart_multi_level` - UART multi-level API (similar pattern)
- `blink_led` - Basic GPIO usage
- `same70_systick_test` - SysTick timer usage

## License

Part of the Alloy Framework - See LICENSE file for details
