# SAME70 Hardware Checklist

Board:
- `same70_xplained`

Build:
- `cmake -S . -B build/hw/same70 -DALLOY_BOARD=same70_xplained -DALLOY_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake`
- `cmake --build build/hw/same70 --target same70_hardware_validation_bundle --parallel 8`

Record:
- date: `2026-04-21`
- operator: `lgili`
- probe/flash flow: `python3 scripts/alloyctl.py flash --board same70_xplained --target <example> --build-first`
- serial port: `/dev/cu.usbmodem1302`

## Stage 1

- `blink`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `validated with temporary fast blink firmware`

- `uart_logger`:
  - flashed: `yes`
  - pass/fail: `fail`
  - notes: `EDBG VCOM still silent; manual USART1 probe also silent; blocker isolated to uart/vcom path`

## Stage 2

- `watchdog_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `500 ms blink stable`

- `rtc_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `500 ms blink after SAME70 RTC handshake fix`

## Stage 3

- `timer_pwm_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `500 ms blink stable`

- `analog_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `1000 ms blink stable; ADC/DAC path validated without UART dependency`

## Stage 4

- `i2c_scan`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `500 ms blink stable`

- `spi_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `500 ms blink after SPI bring-up simplification and clock-enable fix`

## Stage 5

- `dma_probe`:
  - flashed: `no`
  - pass/fail: `blocked`
  - notes: `current observability depends on debug UART; next step is LED-only or memory-visible DMA validation`

## Stage 6

- `can_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `500 ms blink stable`

## Summary

- overall status:
- overall status: `board bring-up and main typed HAL paths validated on hardware except uart/vcom and dma probe`
- blockers:
  - `uart/vcom path on EDBG still silent`
  - `dma_probe` still tied to UART-based observability
- follow-ups:
  - `convert dma_probe to hardware-visible validation without UART`
  - `resume uart/vcom debug with logic analyzer or electrical review of EDBG path`
