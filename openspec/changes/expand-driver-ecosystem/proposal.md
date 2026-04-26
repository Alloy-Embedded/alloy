# Expand Driver Ecosystem — 30+ Sensor and Peripheral Drivers

## Why

Alloy v0.1.0 ships five drivers: SSD1306, BME280, W25Q, KSZ8081, AT24MAC402.
modm has 100+ device drivers. Embassy has 50+ drivers in `embassy-embedded-hal`.
For a developer evaluating HAL libraries, "does it have a driver for my sensor?" is
often the deciding question before architecture quality.

The goal is not to replicate Arduino libraries — it is to provide well-typed, concept-
checked, zero-overhead drivers that demonstrate how the Alloy HAL composes across
different transport layers (I2C, SPI, UART, 1-Wire), and that serve as a reference
implementation for community contributions.

Driver categories:
1. **Environment sensors** — temperature, humidity, pressure, gas, air quality
2. **Motion sensors** — IMU (accel/gyro/mag), GPS
3. **Display** — OLED, e-paper, TFT
4. **Connectivity** — LoRa, CAN transceivers, RS485
5. **Power management** — PMIC, fuel gauges
6. **Storage and ID** — NOR flash, EEPROM, SRAM
7. **Input** — rotary encoder, capacitive touch, keypad

## What Changes

### Driver conventions (`drivers/DRIVER_SPEC.md`)

Before adding drivers, publish a spec that all drivers must follow:
- Header-only unless the driver requires a C library (e.g., codec).
- Templated over HAL transport concept (`SpiPeripheral`, `UartPeripheral`, etc.),
  not over concrete types.
- All methods return `core::Result<T, core::ErrorCode>`. No exceptions.
- A compile-test in `tests/compile_tests/test_driver_<name>.cpp` with a fake transport.
- A probe example in `examples/driver_<name>_probe/` building for at least one board.
- `static_assert` at the bottom of the driver header verifying the concept is satisfied.

### Environment sensors

| Driver | Interface | Notes |
|--------|-----------|-------|
| BME688 | I2C / SPI | Gas sensor, Bosch AI feature set. Successor to BME280. |
| SHT40 | I2C | Sensirion, high-accuracy temp/humidity |
| SHT31 | I2C | Sensirion, industrial grade |
| AHT20 | I2C | Ultra-cheap DHT-replacement |
| LPS22HH | I2C / SPI | STMicro barometer, marine grade |
| CCS811 | I2C | ams, CO2 / TVOC air quality |
| SGP40 | I2C | Sensirion VOC index |
| HDC1080 | I2C | TI, low-power temp/humidity |
| DS18B20 | 1-Wire | Maxim, waterproof temp probe. Requires 1-Wire HAL. |

### Motion and position sensors

| Driver | Interface | Notes |
|--------|-----------|-------|
| ICM-42688-P | SPI / I2C | InvenSense, 6-DoF IMU, best-in-class noise |
| MPU-6050 | I2C | Classic 6-DoF, extremely common |
| LSM6DSOX | I2C / SPI | STMicro 6-DoF with ML core |
| LIS3MDL | I2C / SPI | STMicro 3-axis magnetometer |
| BNO055 | I2C | Bosch 9-DoF with fusion CPU |
| QMC5883L | I2C | Compass, clone of HMC5883L |
| ADXL345 | SPI / I2C | Analog Devices 3-axis accel |
| GNSS / GPS | UART | Generic NMEA-0183 parser (u-blox, L76K) |

### Display drivers

| Driver | Interface | Notes |
|--------|-----------|-------|
| SSD1306 | I2C / SPI | Already exists. Add SPI backend. |
| SH1107 | I2C | 128×64 OLED, square format |
| ST7789 | SPI | 240×320 TFT color display (common on Pico kits) |
| ST7735 | SPI | 128×160 TFT, older format |
| ILI9341 | SPI | 240×320 TFT, very common |
| UC8151 | SPI | 2.13" e-paper, ultra-low power |
| HT16K33 | I2C | 8×8 LED matrix / 7-segment driver |
| TM1637 | GPIO (CLK+DIO) | 4-digit 7-segment display, no I2C |

### Connectivity and protocol

| Driver | Interface | Notes |
|--------|-----------|-------|
| SX1276 | SPI | Semtech LoRa, most common LoRa chip |
| RFM95W | SPI | Hope RF LoRa module (repackaged SX1276) |
| SN65HVD230 | UART + CAN | CAN transceiver config. Works with existing CAN HAL. |
| MAX485 | GPIO | RS485 direction-enable. Already partially in Modbus driver. |
| SI7021 | I2C | Silicon Labs, combined temp/humidity |
| RC522 | SPI | RFID/NFC reader (Mifare) |

### Power management

| Driver | Interface | Notes |
|--------|-----------|-------|
| MAX17048 | I2C | Maxim fuel gauge (LiPo/LiFePO4 SoC%) |
| INA219 | I2C | Texas Instruments current/power monitor |
| INA3221 | I2C | Triple-channel power monitor |
| BQ25895 | I2C | TI LiPo charger with power path |
| TPS63020 | I2C | TI buck-boost converter config |

### Storage and identification

| Driver | Interface | Notes |
|--------|-----------|-------|
| IS25LP128 | SPI | ISSI NOR flash (popular alternative to W25Q) |
| AT25SF128A | SPI | Adesto NOR flash |
| FM25V10 | SPI | Cypress FRAM (no erase cycle limit) |
| 24AA02XEXX | I2C | Microchip EEPROM with EUI-48/64 (variant of existing) |
| DS2431 | 1-Wire | Maxim 1 Kbit EEPROM + 64-bit serial |

### 1-Wire HAL addition

Several high-value sensors (DS18B20, DS2431) use the 1-Wire protocol. A minimal 1-Wire
master HAL requires only a single GPIO pin and precise timing. This is implemented as a
software bit-bang driver in `src/hal/one_wire/` using the `async::gpio::wait_edge`
infrastructure from `complete-async-hal`.

`OneWireMaster<Pin>` concept: `reset()`, `write_bit(bool)`, `read_bit()`. All timing
is derived from `runtime::time`.

### `alloy-codegen` driver scaffold

`alloy-codegen new-driver --name <name> --interface <i2c|spi|uart|1wire>` generates:
- `drivers/<category>/<name>/<name>.hpp` stub with concept boilerplate
- `tests/compile_tests/test_driver_<name>.cpp` stub
- `examples/driver_<name>_probe/main.cpp` stub

Removes the friction of starting a new driver from scratch.

## What Does NOT Change

- Existing five drivers — no API changes; they are extended where appropriate (W25Q
  gains `W25qBlockDevice` adapter in `add-filesystem-hal`; SSD1306 gains SPI backend).
- Core HAL interfaces — drivers only consume existing concepts.

## Alternatives Considered

**Adopt `embedded-hal` driver crates via C FFI:** Rust crates are inaccessible from C++
without a C FFI layer that defeats zero-overhead and type safety. Alloy drivers must be
native C++.

**One driver per PR with no spec:** Without a spec, drivers diverge in style, error
handling, and concept usage within months. `DRIVER_SPEC.md` is mandatory before the
first batch.
