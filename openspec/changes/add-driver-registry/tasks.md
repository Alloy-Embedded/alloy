# Tasks: Driver Registry

Host-testable: phases 1–3. Phase 4 requires hardware.

## 1. Driver concepts

- [ ] 1.1 Create `src/drivers/concepts/` directory.
      Define concepts: `TemperatureSensor`, `PressureSensor`, `HumiditySensor`,
      `AccelerometerSensor`, `Display`, `BlockStorage`, `WirelessModem`.
      Each concept requires `init()` → Result + typed `read_*()` / `write_*()`.
- [ ] 1.2 Create `src/drivers/concepts/hal_requirements.hpp`:
      defines `SpiDriver` and `I2cDriver` concepts that drivers use to
      constrain their HAL handle template parameters.
- [ ] 1.3 Add compile tests: verify mock structs satisfy each concept.

## 2. First-party reference drivers

- [ ] 2.1 Create `src/drivers/ssd1306/ssd1306.hpp` (I2C OLED display driver).
      Implements: `init()`, `clear()`, `draw_text(x, y, string_view)`.
      Satisfies `Display` concept.
- [ ] 2.2 Add `static_assert(Display<Ssd1306<MockI2c>>)`.
- [ ] 2.3 Create `src/drivers/w25q/w25q.hpp` (SPI NOR flash driver).
      Implements: `init()`, `erase(block)`, `write(addr, span)`, `read(addr, span)`.
      Satisfies `BlockStorage` concept.
- [ ] 2.4 Add `examples/ssd1306_hello/` for boards with I2C and OLED.
- [ ] 2.5 Add compile tests for both drivers with mock HAL handles.

## 3. Registry infrastructure

- [ ] 3.1 Create `alloy-rs.dev/registry/drivers.json` (static JSON file in
      `alloy-rs/website` repo or GitHub Pages).
      Seed with: `ssd1306`, `w25q`, `st7735`, `bme280`, `mpu6050`.
- [ ] 3.2 Add `alloy_cli/commands/driver.py`:
      `alloy driver search <query>`, `alloy driver add <id>`, `alloy driver list`.
- [ ] 3.3 Implement `alloy driver add`:
      fetch registry → find driver entry → add FetchContent block to project
      CMakeLists.txt → print usage snippet.
- [ ] 3.4 Implement `alloy driver search`:
      local filter on downloaded registry JSON by id, category, interface keyword.
- [ ] 3.5 Add unit tests for driver registry parsing and search.

## 4. Hardware validation

- [ ] 4.1 nucleo_f401re + SSD1306 OLED (I2C):
      run `examples/ssd1306_hello`; verify "Hello alloy!" appears on display.
- [ ] 4.2 nucleo_f401re + W25Q flash (SPI):
      run `examples/flash_test`; erase + write + read-back 256 bytes; verify data match.

## 5. Documentation

- [ ] 5.1 `docs/DRIVER_REGISTRY.md`: how to use `alloy driver add`, how to write
      a compatible driver, submission process.
- [ ] 5.2 `docs/DRIVER_CONCEPTS.md`: reference for all concept interfaces with
      examples of concept-satisfying implementations.
- [ ] 5.3 Create template repo `alloy-rs/driver-template` with boilerplate for a
      new driver (concepts, compile test, CMakeLists, README).
