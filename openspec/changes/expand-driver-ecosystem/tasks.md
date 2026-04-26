# Tasks: Expand Driver Ecosystem

All driver phases are host-testable (concept checks + compile tests with fake transports).
Hardware spot-checks are listed where a reference board is available.

> **Baseline** — v0.1.0 ships: SSD1306, BME280, W25Q (+ BlockDevice adapter),
> AT24MAC402, KSZ8081, IS42S16100F, SdCard, LittlefsBackend, FatfsBackend.

## 1. Driver spec and scaffold tool

- [ ] 1.1 Create `drivers/DRIVER_SPEC.md`: coding conventions, required `static_assert`,
      Result-only error handling, forbidden patterns (heap, exceptions, `printf`),
      required test and example stubs.
- [ ] 1.2 Implement `alloyctl new-driver --name <n> --interface <i2c|spi|uart|1wire>`:
      generates driver header stub, compile test stub, and example stub.
- [ ] 1.3 Add CI job `driver-spec-lint`: checks all driver headers for required
      `static_assert` line and disallowed patterns (`new`, `malloc`, `throw`).

## 2. 1-Wire HAL

- [ ] 2.1 `src/hal/one_wire/one_wire.hpp`: `OneWireMaster<T>` concept.
      `reset() → Result<bool>`, `write_bit(bool)`, `read_bit() → bool`.
- [ ] 2.2 `src/hal/one_wire/one_wire_gpio.hpp`: `OneWireGpio<Pin>` bit-bang.
      Timing: 480 µs reset, 60 µs bit slot via `runtime::time` calibrated delays.
- [ ] 2.3 `tests/compile_tests/test_hal_one_wire_concept.cpp`: concept check.

## 3. Environment sensor batch

- [x] 3.1 `drivers/sensor/bme280/bme280.hpp` — BME280 over I2C. Chip-ID, calibration,
      compensated T/P/H. ✅ compile-review.
- [ ] 3.2 `drivers/sensor/bme688/bme688.hpp` — BME688 over I2C/SPI. Raw T/P/H/G read,
      Bosch compensation formulas. IAQ deferred (requires BSEC, proprietary).
- [x] 3.3 `drivers/sensor/sht4x/sht4x.hpp` — SHT40/SHT41 over I2C. Single-shot
      measurement, CRC-8 verification. ✅ compile-review. HW validation pending.
- [x] 3.4 `drivers/sensor/aht20/aht20.hpp` — AHT20 over I2C. Init + trigger + read,
      20-bit raw → °C and %RH. ✅ compile-review. HW validation pending.
- [x] 3.5 `drivers/sensor/lps22hh/lps22hh.hpp` — LPS22HH barometer over I2C.
      WHO_AM_I probe, one-shot P+T, 24-bit signed pressure, 16-bit signed temp.
      ✅ compile-review. HW validation pending.
- [ ] 3.6 `drivers/sensor/ccs811/ccs811.hpp` — CCS811 CO₂/TVOC over I2C.
      App firmware boot sequence, NDRDY polling.
- [ ] 3.7 `drivers/sensor/sgp40/sgp40.hpp` — SGP40 VOC index over I2C.
      Measure raw command, CRC-8 verification.
- [ ] 3.8 `drivers/sensor/ds18b20/ds18b20.hpp` — DS18B20 over 1-Wire.
      ROM search (single device), convert + read temperature. Depends on task 2.

## 4. Motion sensor batch

- [x] 4.1 `drivers/sensor/icm42688p/icm42688p.hpp` — ICM-42688-P over SPI.
      WHO_AM_I, soft-reset, PWR_MGMT0, configurable accel/gyro FS + ODR, 14-byte burst.
      ✅ compile-review. HW validation pending.
- [x] 4.2 `drivers/sensor/mpu6050/mpu6050.hpp` — MPU-6050 over I2C.
      WHO_AM_I, reset+wake, DLPF, configurable accel/gyro FS, 14-byte burst read.
      ✅ compile-review. HW validation pending.
- [x] 4.3 `drivers/sensor/lsm6dsox/lsm6dsox.hpp` — LSM6DSOX over I2C.
      WHO_AM_I (0x6C), IF_INC, 416 Hz ODR, configurable FS, 14-byte burst read.
      ✅ compile-review. HW validation pending.
- [x] 4.4 `drivers/sensor/lis3mdl/lis3mdl.hpp` — LIS3MDL magnetometer over I2C.
      WHO_AM_I (0x3D), ultra-high perf, STATUS poll, 6-byte burst read.
      ✅ compile-review. HW validation pending.
- [x] 4.5 `drivers/net/nmea_parser/nmea_parser.hpp` — NMEA-0183 GPGGA/GPRMC parser.
      Stateless `Parser<BusHandle>`, byte-by-byte UART reader, no heap, no stdlib strings.
      ✅ compile-review. HW validation pending.

## 5. Display driver batch

- [x] 5.1 `drivers/display/ssd1306/ssd1306.hpp` — SSD1306 over I2C. Init, clear,
      pixel, text, flush. ✅ compile-review. HW validated (AT24MAC402 probe board).
- [x] 5.2 SSD1306 SPI transport backend — `SpiDevice<Spi, DcPolicy, CsPolicy>` added
      alongside existing I2C `Device`. Uses `transfer(tx,rx)` bus surface.
      ✅ compile-review. HW validation pending.
- [x] 5.3 `drivers/display/st7789/st7789.hpp` — ST7789 over SPI. Init, fill_rect,
      draw_pixel, blit. DcPolicy + CsPolicy templates. 64-byte stack chunks (no heap).
      ✅ compile-review. HW validation pending.
- [x] 5.4 `drivers/display/ili9341/ili9341.hpp` — ILI9341 over SPI. Same API as ST7789,
      11-step init sequence per datasheet.
      ✅ compile-review. HW validation pending.
- [x] 5.5 `drivers/display/uc8151/uc8151.hpp` — UC8151 e-paper over SPI.
      Full update, BUSY pin poll, deep-sleep. Width/Height as template params.
      ✅ compile-review. HW validation pending.
- [x] 5.6 `drivers/display/tm1637/tm1637.hpp` — TM1637 4-digit 7-seg over GPIO bit-bang.
      LSB-first 2-wire protocol, display_number, set_brightness, clear.
      ✅ compile-review. HW validation pending.

## 6. Power management and monitoring

- [ ] 6.1 `drivers/power/max17048/max17048.hpp` — MAX17048 LiPo fuel gauge over I2C.
      SOC%, voltage, charge rate, alert threshold.
- [ ] 6.2 `drivers/power/ina219/ina219.hpp` — INA219 power monitor over I2C.
      Calibration register, shunt + bus voltage, power, current.
- [ ] 6.3 `drivers/power/ina3221/ina3221.hpp` — INA3221 triple-channel over I2C.
      All three channels in one transaction.

## 7. Connectivity drivers

- [ ] 7.1 `drivers/net/sx1276/sx1276.hpp` — SX1276 LoRa over SPI.
      Implicit/explicit header, configurable SF/BW/CR, TX + RX with DIO0 interrupt.
      No LoRaWAN stack.
- [ ] 7.2 `drivers/net/rc522/rc522.hpp` — MFRC522 RFID over SPI.
      ISO 14443-A init, request, anti-collision, read UID.

## 8. Storage drivers

- [x] 8.1 `drivers/memory/w25q/w25q.hpp` — W25Q NOR flash over SPI. JEDEC, read,
      page program, sector erase. ✅ compile-review. HW validation pending.
- [x] 8.2 `drivers/memory/w25q/w25q_block_device.hpp` — BlockDevice adapter.
      ✅ compile-review. HW validation pending.
- [x] 8.3 `drivers/memory/at24mac402/at24mac402.hpp` — AT24MAC402 EEPROM over I2C.
      Paged read/write, EUI-48/64 + 128-bit serial. ✅ compile-review.
      HW validation pending (see project_network_hw_validation.md).
- [x] 8.4 `drivers/memory/sdcard/sdcard.hpp` — SD card SPI-mode driver.
      CMD0/8/ACMD41/58 init, CMD17/24 block R/W, SDHC/SDXC only.
      ✅ compile-review. HW validation pending.
- [ ] 8.5 `drivers/memory/fm25v10/fm25v10.hpp` — Cypress FRAM over SPI.
      No erase cycle limit. Read/write at byte granularity.

## 9. CI and registry

- [ ] 9.1 `compile-review-drivers` CI job: builds all driver compile tests in parallel.
      Blocks on any new driver PR that lacks a compile test.
- [ ] 9.2 `drivers/MANIFEST.json`: machine-readable registry. Fields: `name`, `category`,
      `interface`, `chips`, `status`, `example`, `notes`.
- [ ] 9.3 `docs/DRIVERS.md`: driver catalog auto-generated from `MANIFEST.json`.
      Status: compile-review / hardware-validated.
- [ ] 9.4 `alloyctl info --drivers`: lists available drivers and status from MANIFEST.json.
