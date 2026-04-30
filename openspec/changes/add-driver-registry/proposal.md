# Proposal: Driver Registry

## Status
`open` — enables ecosystem growth beyond the core HAL.

## Problem

alloy has HAL modules (gpio, uart, spi, i2c, ...) but no support for
off-chip peripheral drivers (displays, sensors, motor controllers, wireless
modules). Users building real products need:

- SPI displays (ST7735, ILI9341, SSD1306)
- I2C sensors (BME280, MPU-6050, VL53L0X)
- NOR/NAND flash (W25Q, S25FL)
- Wireless (ESP-AT modem, LoRa SX127x, nRF24L01)

Without a registry, users write drivers from scratch, copy-paste from random
GitHub repos with no alloy integration, or import Arduino libraries that bypass
the HAL entirely.

## Proposed Solution

### Driver as a header-only template library

A driver is a C++20 template that accepts alloy HAL handles as template parameters:

```cpp
// Driver concept: a driver takes HAL handle(s) + config, exposes a typed API.
template <typename SpiHandle, typename CsPin>
class St7735 {
public:
    using Config = St7735Config;  // width, height, color order, invert

    explicit St7735(SpiHandle& spi, CsPin& cs, const Config& cfg = {});

    auto init()                              -> core::Result<void, core::ErrorCode>;
    auto fill(uint16_t color)                -> core::Result<void, core::ErrorCode>;
    auto draw_pixel(int16_t x, int16_t y, uint16_t color) -> core::Result<void, core::ErrorCode>;
    auto draw_image(int16_t x, int16_t y,
                    uint16_t w, uint16_t h,
                    std::span<const uint16_t> pixels) -> core::Result<void, core::ErrorCode>;
};
```

No virtual dispatch. No heap. SpiHandle and CsPin are duck-typed via
C++20 concept constraints defined in `alloy::drivers::concepts`.

### Driver registry JSON

`https://alloy-rs.dev/registry/drivers.json`:

```json
{
  "version": 1,
  "drivers": [
    {
      "id": "st7735",
      "display_name": "ST7735 TFT display",
      "category": "display",
      "interface": ["spi"],
      "url": "https://github.com/alloy-rs/driver-st7735",
      "version": "0.2.0",
      "alloy_min_version": "0.5.0",
      "verified_boards": ["nucleo_f401re", "raspberry_pi_pico"]
    },
    {
      "id": "bme280",
      "display_name": "Bosch BME280 env sensor",
      "category": "sensor",
      "interface": ["i2c", "spi"],
      "url": "https://github.com/alloy-rs/driver-bme280",
      "version": "0.3.1",
      "alloy_min_version": "0.5.0"
    }
  ]
}
```

### `alloy driver` CLI commands

```sh
alloy driver search bme280
# [match] bme280 — Bosch BME280 env sensor (i2c/spi) v0.3.1

alloy driver add bme280
# Clones driver-bme280 into project's drivers/ directory
# Adds CMake FetchContent block to CMakeLists.txt
# Adds #include comment to src/main.cpp

alloy driver list
# Lists all drivers added to current project (reads CMakeLists.txt)
```

### `DriverConcept` — driver interface contracts

Each driver category has a concept ensuring type-safe inter-driver composition:

```cpp
// src/drivers/concepts/sensor.hpp
template <typename T>
concept TemperatureSensor = requires(T& t) {
    { t.read_temperature() } -> std::same_as<core::Result<float, core::ErrorCode>>;
};

template <typename T>
concept PressureSensor = requires(T& t) {
    { t.read_pressure() } -> std::same_as<core::Result<float, core::ErrorCode>>;
};
```

### Verification workflow

Community-submitted drivers are accepted via PR to `alloy-rs/driver-<name>`.
CI runs:
1. Compile test against the latest alloy HAL.
2. Concept-satisfaction `static_assert`.
3. (If hardware available) functional test on verified boards.

Verified drivers appear in the registry with `verified_boards` populated.

## Non-goals

- alloy does not maintain the driver implementations — each driver is a separate repo.
- No binary distribution of drivers.
- No Arduino library compatibility layer.
