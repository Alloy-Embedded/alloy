# SAME70 Hardware Checklist

Board:
- `same70_xplained`

Build:
- `cmake -S . -B build/hw/same70 -DALLOY_BOARD=same70_xplained -DALLOY_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake`
- `cmake --build build/hw/same70 --target same70_hardware_validation_bundle --parallel 8`

Record:
- date: `2026-04-22`
- operator: `lgili`
- probe/flash flow: `python3 scripts/alloyctl.py flash --board same70_xplained --target <example> --build-first`
- serial port: `/dev/cu.usbmodem11102`
- terminal: `115200 8N1`

## Stage 1

- `blink`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `validated on final firmware with UART heartbeat: blink loop=...`

- `uart_logger`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `clean INFO/WARN/ERROR output on EDBG VCOM via USART1 PB4/PA21 at 115200`

## Stage 2

- `watchdog_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART heartbeat stable: watchdog refresh=...`

- `rtc_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART heartbeat stable: rtc loop=... after SAME70 RTC handshake fix`

## Stage 3

- `timer_pwm_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART heartbeat stable: timer/pwm loop=...`

- `analog_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART heartbeat stable: analog loop=...; ADC/DAC path validated`

## Stage 4

- `i2c_scan`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART output stable: i2c scan none on empty bus`

- `spi_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART heartbeat stable: spi loop=... after SPI bring-up simplification and clock-enable fix`

## Stage 5

- `dma_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART heartbeat stable: dma loop=...; DMA binding/config path validated on hardware`

## Stage 6

- `can_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `UART heartbeat stable: can loop=...`

## Summary

- overall status: `board bring-up, debug uart, and typed HAL paths validated on hardware`
- overall status: `all SAME70 validation-bundle examples flash, boot, and emit stable UART telemetry on hardware`
- blockers:
  - `none in current SAME70 validation bundle`
- follow-ups:
  - `repeat the same flash+monitor sweep on stm32g0 and stm32f4`
  - `add deeper peripheral traffic assertions on top of the now-stable UART-backed probes`
