## 1. OpenSpec Baseline

- [x] 1.1 Add new `driver-seed-library` capability with requirements covering the convention
      and the three seeded drivers

## 2. Driver Library Infrastructure

- [x] 2.1 Add `drivers/` top-level directory with `README.md` documenting the convention
- [x] 2.2 Add opt-in `drivers/CMakeLists.txt` + entry in top-level `CMakeLists.txt`

## 3. SSD1306 Seed

- [x] 3.1 Add `drivers/display/ssd1306/ssd1306.hpp` + 5x7 font
- [x] 3.2 Add compile test `tests/compile_tests/test_driver_seed_ssd1306.cpp`

## 4. BME280 Seed

- [x] 4.1 Add `drivers/sensor/bme280/bme280.hpp` with calibration load + compensated reads
- [x] 4.2 Add compile test `tests/compile_tests/test_driver_seed_bme280.cpp`

## 5. W25Q Seed

- [x] 5.1 Add `drivers/memory/w25q/w25q.hpp` with JEDEC ID, read, program, erase, wait
- [x] 5.2 Add compile test `tests/compile_tests/test_driver_seed_w25q.cpp`

## 6. Validation

- [x] 6.1 Register the three compile tests in `tests/compile/contract_smoke.cmake`
- [x] 6.2 `openspec validate seed-driver-library --strict`
