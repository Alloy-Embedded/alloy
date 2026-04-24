## Why

The v0.1 release proves the public HAL works on silicon, but it does not yet prove the HAL
is *pleasant* to build on top of. Competing ecosystems (Zephyr, ESP-IDF, PlatformIO, Arduino)
ship hundreds of device drivers out of the box. Without at least a seed driver library,
adopters have nothing to copy from when wiring their first display, sensor, or external flash,
and the "is the public HAL surface real?" question has no third-party answer.

A seed library of three canonical peripherals covers the three most common bring-up patterns:

- **SSD1306** (I2C, display) — proves the I2C read/write/write_read surface is usable for a
  command/data protocol with framebuffer flush
- **BME280** (I2C, environmental sensor) — proves the HAL supports compensation-math drivers
  where the device publishes calibration data and the driver performs integer math on it
- **W25Q** (SPI, NOR flash) — proves the SPI chip-select + transfer surface is usable for a
  block-device protocol with status polling

## What Changes

- Add a new `drivers/` top-level directory with a driver-library convention: header-only,
  templated on the HAL handle type, `core::Result<T, core::ErrorCode>` everywhere, no dynamic
  allocation, no ownership of the bus.
- Implement three seed drivers:
  - `drivers/display/ssd1306/` — 128x64 OLED display over I2C with init, clear, draw_pixel,
    draw_text (5x7 font), and framebuffer flush
  - `drivers/sensor/bme280/` — temperature, pressure, humidity sensor over I2C with chip-ID
    check, calibration-data load, and compensated reads
  - `drivers/memory/w25q/` — Winbond W25Q-series NOR flash over SPI with JEDEC ID check,
    page read, page program, sector erase, and status wait
- Add compile tests under `tests/compile_tests/` that instantiate each driver against the
  public HAL handle surface, so the contract stays honest.
- Document the driver pattern in `drivers/README.md` so the next driver contributor has a
  template to copy.
- Add a new `driver-seed-library` OpenSpec capability recording the pattern and the three
  seeded drivers.

Out of scope for this change:

- full hardware examples per driver (those require peripheral-specific wiring and lab time;
  they are follow-up changes per foundational board)
- exhaustive feature coverage (SSD1306 scroll, BME280 IIR filter tuning, W25Q chip erase or
  security registers) — the seed covers the most common bring-up surface only

## Outcome

Adopters have three working drivers to copy from, and the public HAL surface has three
third-party-style consumers that exercise I2C-write, I2C-write-read with compensation math,
and SPI full-duplex transfers. New drivers can follow the documented pattern without inventing
conventions.

## Impact

- Affected specs:
  - new: `driver-seed-library`
- Affected code and docs:
  - `drivers/` (new top-level directory)
  - `drivers/README.md` (new)
  - `drivers/display/ssd1306/ssd1306.hpp` (new)
  - `drivers/display/ssd1306/font_5x7.hpp` (new)
  - `drivers/sensor/bme280/bme280.hpp` (new)
  - `drivers/memory/w25q/w25q.hpp` (new)
  - `tests/compile_tests/test_driver_seed_*.cpp` (new)
  - `tests/compile/contract_smoke.cmake` (registers the new compile tests)
  - `CMakeLists.txt` (opt-in `add_subdirectory(drivers)`)
