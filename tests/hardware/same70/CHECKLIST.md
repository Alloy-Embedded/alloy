# SAME70 Hardware Checklist

Board:
- `same70_xplained`

Build:
- `cmake -S . -B build/hw/same70 -DALLOY_BOARD=same70_xplained -DALLOY_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake`
- `cmake --build build/hw/same70 --target same70_hardware_validation_bundle --parallel 8`

Record:
- date:
- operator:
- probe/flash flow:
- serial port:

## Stage 1

- `blink`:
  - flashed:
  - pass/fail:
  - notes:

- `uart_logger`:
  - flashed:
  - pass/fail:
  - notes:

## Stage 2

- `watchdog_probe`:
  - flashed:
  - pass/fail:
  - notes:

- `rtc_probe`:
  - flashed:
  - pass/fail:
  - notes:

## Stage 3

- `timer_pwm_probe`:
  - flashed:
  - pass/fail:
  - notes:

- `analog_probe`:
  - flashed:
  - pass/fail:
  - notes:

## Stage 4

- `i2c_scan`:
  - flashed:
  - pass/fail:
  - notes:

- `spi_probe`:
  - flashed:
  - pass/fail:
  - notes:

## Stage 5

- `dma_probe`:
  - flashed:
  - pass/fail:
  - notes:

## Stage 6

- `can_probe`:
  - flashed:
  - pass/fail:
  - notes:

## Summary

- overall status:
- blockers:
- follow-ups:
