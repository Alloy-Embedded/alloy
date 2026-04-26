# Tasks: Expand Driver Ecosystem

All driver phases are host-testable (concept checks + unit tests with fake transports).
Hardware spot-checks are listed where a reference board is available.

## 1. Driver spec and scaffold tool

- [ ] 1.1 Create `drivers/DRIVER_SPEC.md`: coding conventions, required `static_assert`,
      Result-only error handling, forbidden patterns (heap, exceptions, `printf`),
      required test and example stubs.
- [ ] 1.2 Implement `alloy-codegen new-driver --name <n> --interface <i2c|spi|uart|1wire>`:
      generates driver header stub, compile test stub, and example stub following the spec.
- [ ] 1.3 Add CI job `driver-spec-lint`: checks all driver headers in `drivers/` for
      required `static_assert` line and disallowed patterns (`new`, `malloc`, `throw`).

## 2. 1-Wire HAL

- [ ] 2.1 Create `src/hal/one_wire/one_wire.hpp`: `OneWireMaster<T>` concept.
      `reset() → Result<bool>` (presence pulse), `write_bit(bool)`, `read_bit() → bool`.
- [ ] 2.2 Create `src/hal/one_wire/one_wire_gpio.hpp`: `OneWireGpio<Pin>` bit-bang
      implementation. Timing uses `runtime::time` with calibrated delays (480 µs reset,
      60 µs bit slot). Requires GPIO output + input switching on same pin.
- [ ] 2.3 `tests/compile_tests/test_hal_one_wire_concept.cpp`: concept check.

## 3. Environment sensor batch

- [ ] 3.1 `drivers/sensor/bme688/bme688.hpp`: BME688 over I2C/SPI. Read raw T/P/H/G,
      apply Bosch compensation formulas. IAQ index calculation deferred (requires BSEC
      library, proprietary). `static_assert` + compile test + probe example.
- [ ] 3.2 `drivers/sensor/sht4x/sht4x.hpp`: SHT40/SHT41 over I2C. Single-shot
      measurement command, CRC-8 verification. Probe example targeting `nucleo_g071rb`.
- [ ] 3.3 `drivers/sensor/aht20/aht20.hpp`: AHT20 over I2C. Init + trigger + read
      sequence, 20-bit raw → physical conversion.
- [ ] 3.4 `drivers/sensor/lps22hh/lps22hh.hpp`: LPS22HH over I2C. Pressure + temp
      output data registers, FIFO mode optional.
- [ ] 3.5 `drivers/sensor/ccs811/ccs811.hpp`: CCS811 over I2C. App firmware boot
      sequence, NDRDY polling or interrupt.
- [ ] 3.6 `drivers/sensor/sgp40/sgp40.hpp`: SGP40 over I2C. Measure raw VOC command,
      CRC verification.
- [ ] 3.7 `drivers/sensor/ds18b20/ds18b20.hpp`: DS18B20 over 1-Wire. ROM search
      (single device), convert + read temperature, parasitic power support flag.
      Compile test + example.

## 4. Motion sensor batch

- [ ] 4.1 `drivers/sensor/icm42688p/icm42688p.hpp`: ICM-42688-P over SPI. Accel + gyro
      raw read, configurable ODR, FIFO mode, WHO_AM_I check.
- [ ] 4.2 `drivers/sensor/mpu6050/mpu6050.hpp`: MPU-6050 over I2C. Accel + gyro +
      temp read, configurable full-scale range.
- [ ] 4.3 `drivers/sensor/lsm6dsox/lsm6dsox.hpp`: LSM6DSOX over I2C/SPI. 6-DoF
      with embedded functions (step counter, significant motion) disabled by default.
- [ ] 4.4 `drivers/sensor/lis3mdl/lis3mdl.hpp`: LIS3MDL over I2C/SPI. 3-axis mag,
      configurable full-scale.
- [ ] 4.5 `drivers/sensor/nmea/nmea_parser.hpp`: NMEA-0183 parser (GPGGA, GPRMC).
      Stateless, line-by-line. Template over `UartPeripheral`. No heap.

## 5. Display driver batch

- [ ] 5.1 `drivers/display/ssd1306/ssd1306.hpp`: add SPI transport backend to existing
      I2C-only driver. Template parameter selects transport.
- [ ] 5.2 `drivers/display/st7789/st7789.hpp`: ST7789 over SPI. Init sequence, fill
      rect, draw pixel, VSYNC optional. Framebuffer is user-supplied `std::span`.
- [ ] 5.3 `drivers/display/ili9341/ili9341.hpp`: ILI9341 over SPI. Same API as ST7789.
- [ ] 5.4 `drivers/display/uc8151/uc8151.hpp`: UC8151 e-paper over SPI. Full update +
      partial update, busy-wait on BUSY pin. Low-power display off.
- [ ] 5.5 `drivers/display/tm1637/tm1637.hpp`: TM1637 4-digit 7-segment over GPIO
      bit-bang (CLK + DIO). Display number, brightness control.
- [ ] 5.6 Probe examples for ST7789 (targeting `raspberry_pi_pico`) and e-paper
      UC8151 (targeting `nucleo_g071rb` + breakout).

## 6. Power management and monitoring

- [ ] 6.1 `drivers/power/max17048/max17048.hpp`: MAX17048 fuel gauge over I2C. State
      of charge (SOC%), voltage, charge rate. Alert threshold config.
- [ ] 6.2 `drivers/power/ina219/ina219.hpp`: INA219 power monitor over I2C. Config
      calibration register, read shunt voltage, bus voltage, power, current.
- [ ] 6.3 `drivers/power/ina3221/ina3221.hpp`: INA3221 triple-channel over I2C. All
      three channel reads in one transaction.
- [ ] 6.4 Compile tests + probe examples for INA219 (i2c_scan board as base).

## 7. Connectivity drivers

- [ ] 7.1 `drivers/net/sx1276/sx1276.hpp`: SX1276 LoRa over SPI. Implicit / explicit
      header mode, configurable SF/BW/CR, TX + RX with DIO0 interrupt. No LoRaWAN
      stack (that is a higher layer).
- [ ] 7.2 `drivers/net/rc522/rc522.hpp`: MFRC522 RFID over SPI. ISO 14443-A init,
      request, anti-collision, read UID.
- [ ] 7.3 Probe examples: `driver_sx1276_probe` (LoRa TX ping), `driver_rc522_probe`
      (read NFC card UID to UART). Build for `nucleo_g071rb`.

## 8. CI and registry

- [ ] 8.1 `compile-review-drivers` CI job: builds all driver compile tests in parallel.
      Blocks on any new driver PR that lacks a compile test.
- [ ] 8.2 `docs/DRIVERS.md`: driver catalog with interface, chip, status (compile-review
      / hardware-validated), and link to example. Auto-generated from a JSON manifest
      in `drivers/MANIFEST.json`.
- [ ] 8.3 `drivers/MANIFEST.json`: machine-readable driver registry. Fields: `name`,
      `category`, `interface`, `chips`, `status`, `example`, `notes`. Used by
      `alloyctl info --drivers` and the docs auto-gen script.
- [ ] 8.4 `alloyctl info --drivers`: lists all available drivers and their status.
      Reads from the locally installed `drivers/MANIFEST.json`.
