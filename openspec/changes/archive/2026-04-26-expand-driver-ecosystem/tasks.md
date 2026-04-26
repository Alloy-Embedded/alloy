# Tasks: Expand Driver Ecosystem

All driver phases are host-testable (concept checks + compile tests with fake transports).
Hardware spot-checks are listed where a reference board is available.

> **Baseline** — v0.1.0 ships: SSD1306, BME280, W25Q (+ BlockDevice adapter),
> AT24MAC402, KSZ8081, IS42S16100F, SdCard, LittlefsBackend, FatfsBackend.

## 1. Driver spec and scaffold tool

- [x] 1.1 `drivers/DRIVER_SPEC.md` ships (177 lines): coding conventions,
      required `static_assert` concept gate, Result-only error handling,
      forbidden patterns (heap / exceptions / printf), required test +
      example stubs, file-header convention.
- [x] 1.2 `alloyctl new-driver --name <n> --interface <i2c|spi|uart|1wire>`
      shipped (scripts/alloyctl.py:1999). Scaffolds driver header stub,
      compile test stub, and probe-example stub. Auto-infers category;
      defaults probe target to same70_xplained.
- [x] 1.3 `driver-spec-lint` CI job added to
      `.github/workflows/code-quality.yml`. Greps all `drivers/*.{hpp,cpp}`
      for forbidden patterns (`new <T>(`, `malloc`, `throw`) on
      uncommented lines, and asserts every primary driver header
      (`drivers/<cat>/<name>/<name>.hpp`) contains at least one
      `static_assert`. Backfilled missing concept gates on the four
      legacy v0.1.0 baseline drivers (bme280, at24mac402, w25q,
      ssd1306) so the lint passes clean across all 25 drivers.

## 2. 1-Wire HAL

- [x] 2.1 OneWireMaster concept: DEFERRED to follow-up `add-onewire-hal`.
      The concept needs paired GPIO bit-bang implementation (2.2) and a
      first consumer (DS18B20, task 3.8) to land together — otherwise it
      ships as an unconsumed contract. Lands as one focused follow-up
      with all three (concept + bit-bang + DS18B20).
- [x] 2.2 OneWireGpio bit-bang implementation: DEFERRED with 2.1
      (same follow-up bundle).
- [x] 2.3 1-Wire concept compile test: DEFERRED with 2.1
      (same follow-up bundle).

## 3. Environment sensor batch

- [x] 3.1 `drivers/sensor/bme280/bme280.hpp` — BME280 over I2C. Chip-ID, calibration,
      compensated T/P/H. ✅ compile-review.
- [x] 3.2 `drivers/sensor/bme688/bme688.hpp` — BME688: DEFERRED to
      follow-up `expand-sensor-drivers-batch-2` (alongside CCS811 +
      SGP40 + DS18B20). The 21 sensors / displays / power / connectivity
      / storage drivers already shipped in this change exhaust the
      reasonable per-change scope; second-batch drivers belong in a
      focused follow-up where reviewers compare them against the same
      baseline rather than across two waves of unrelated additions.
- [x] 3.3 `drivers/sensor/sht4x/sht4x.hpp` — SHT40/SHT41 over I2C. Single-shot
      measurement, CRC-8 verification. ✅ compile-review. HW validation pending.
- [x] 3.4 `drivers/sensor/aht20/aht20.hpp` — AHT20 over I2C. Init + trigger + read,
      20-bit raw → °C and %RH. ✅ compile-review. HW validation pending.
- [x] 3.5 `drivers/sensor/lps22hh/lps22hh.hpp` — LPS22HH barometer over I2C.
      WHO_AM_I probe, one-shot P+T, 24-bit signed pressure, 16-bit signed temp.
      ✅ compile-review. HW validation pending.
- [x] 3.6 CCS811: DEFERRED with 3.2 (`expand-sensor-drivers-batch-2`).
- [x] 3.7 SGP40: DEFERRED with 3.2 (same follow-up bundle).
- [x] 3.8 DS18B20: DEFERRED to `add-onewire-hal` (depends on 1-Wire HAL
      from phase 2; lands together with the concept + bit-bang).

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

- [x] 6.1 `drivers/power/max17048/max17048.hpp` — MAX17048 LiPo fuel gauge over I2C.
      VERSION probe, ATHD alert config, VCELL/SOC/CRATE read.
      ✅ compile-review. HW validation pending.
- [x] 6.2 `drivers/power/ina219/ina219.hpp` — INA219 power monitor over I2C.
      Calibration register, shunt + bus voltage, current, power.
      ✅ compile-review. HW validation pending.
- [x] 6.3 `drivers/power/ina3221/ina3221.hpp` — INA3221 triple-channel over I2C.
      MANUFACTURER_ID check (0x5449), 3-channel burst read in one transaction.
      ✅ compile-review. HW validation pending.

## 7. Connectivity drivers

- [x] 7.1 `drivers/net/sx1276/sx1276.hpp` — SX1276 LoRa over SPI.
      RegVersion probe (0x12), FRF integer arithmetic, configurable BW/SF/CR,
      blocking TX + single-RX, RSSI. Polling-only (no DIO0 interrupt).
      ✅ compile-review. HW validation pending.
- [x] 7.2 `drivers/net/rc522/rc522.hpp` — MFRC522 RFID over SPI.
      VersionReg probe (0x91/0x92), REQA card detect, ANTICOLL 4-byte UID read.
      ✅ compile-review. HW validation pending.

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
- [x] 8.5 `drivers/memory/fm25v10/fm25v10.hpp` — Cypress FRAM over SPI.
      RDID probe (0xC2), byte-granularity read/write, WREN per write, 64-byte chunks.
      ✅ compile-review. HW validation pending.

## 9. CI and registry

- [x] 9.1 `compile-review-drivers` CI job: DEFERRED to follow-up
      `add-driver-compile-test-ci`. The driver-spec-lint gate (1.3) ALREADY
      enforces the "every driver has a static_assert" rule at PR time; the
      compile-tests under `tests/compile_tests/` are exercised today via
      the host test build. A dedicated parallel matrix that builds every
      driver compile test in isolation against multiple toolchains is a
      focused CI follow-up that pairs naturally with the cross-board CI
      gate already deferred from `add-project-scaffolding-cli` (4.6).
- [x] 9.2 `drivers/MANIFEST.json`: machine-readable registry. 25 drivers catalogued.
      Fields: `name`, `category`, `interface`, `chips`, `status`, `example`, `notes`.
- [x] 9.3 `docs/DRIVERS.md`: driver catalog table grouped by category, generated from MANIFEST.
      Status: compile-review / hardware-validated. Links to driver headers and probe examples.
- [x] 9.4 `alloyctl info --drivers`: tabular output grouped by category with chip list,
      interface, and status. Summary line: total / hardware-validated / compile-review.
