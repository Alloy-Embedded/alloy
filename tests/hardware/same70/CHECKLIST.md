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

---

## Record

- date: `2026-04-24`
- operator: `codex + lgili`
- probe/flash flow:
  - `python3 scripts/alloyctl.py flash --board same70_xplained --target <example>`
  - `python3 scripts/alloyctl.py monitor --board same70_xplained --port /dev/cu.usbmodem1102`
  - for boot-sensitive probes: `monitor-first + openocd reset run`
- serial port: `/dev/cu.usbmodem1102`
- terminal: `115200 8N1`

## 2026-04-24 Results

- `blink`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable UART heartbeat: blink loop=...`

- `uart_logger`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable INFO/WARN/ERROR output after rerun with monitor-first + reset`

- `time_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable timeout telemetry after rerun: timeout window=...`

- `watchdog_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable watchdog refresh telemetry after rerun: watchdog refresh=...`

- `rtc_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable UART loop telemetry: rtc loop=...`

- `timer_pwm_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable timer/pwm loop telemetry after rerun: timer/pwm loop=...`

- `analog_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable analog loop telemetry with DAC active banner`

- `i2c_scan`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `scan completed and reported i2c scan none on current setup`

- `spi_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `stable SPI loop telemetry: spi loop=...`

- `dma_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `boot banner and binding logs captured after rerun: dma probe ready / dma bindings configured`

- `can_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `boot banner and loop telemetry captured: can probe ready / can configured / can loop=...`

- `manual_uart_probe`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `manual usart1 pb4/pa21 115200 8n1 visible on onboard VCOM`

- `same70_uart_probe_usart1_pb4_pa21`:
  - flashed: `yes`
  - pass/fail: `pass`
  - notes: `typed public route probe emitted same70 probe USART1 PB4/PA21 on onboard VCOM`

- `same70_uart_probe_usart0_pb01`:
  - flashed: `build-only`
  - pass/fail: `not-observable on current lab wiring`
  - notes: `UART route exits via PB1/PB0, not via onboard EDBG VCOM`

- `same70_uart_probe_uart1_pa56`:
  - flashed: `build-only`
  - pass/fail: `not-observable on current lab wiring`
  - notes: `UART route exits via PA6/PA5, not via onboard EDBG VCOM`

## 2026-04-24 Summary

- overall status: `all observable SAME70 examples passed on hardware`
- blockers:
  - `the two alternate uart_path_probe variants need external wiring or a second USB-UART adapter for honest hardware validation`
- follow-ups:
  - `add an alloyctl hardware sweep mode that waits for serial reopen and can reset after monitor attach`
