# SAME70 Hardware Validation Bundle

Representative board:

- `same70_xplained`

## Why This Board

- first Renode family already validated in Alloy
- richest current board support layer in the repo
- exposes public HAL paths for UART, I2C, SPI, DMA, timer, PWM, ADC, DAC, RTC, CAN, and watchdog

## Required Equipment

- SAME70 Xplained Ultra board
- board power/debug connection through the on-board debugger used by the lab
- serial terminal attached to the board debug UART path

The repo does not hardcode a flashing tool here. Use the on-board debugger or external probe flow
already used for SAME70 boards in the lab.

## Configure And Build

```bash
cmake -S . -B build/hw/same70 \
  -DALLOY_BOARD=same70_xplained \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/same70 --target same70_hardware_validation_bundle --parallel 8
```

Artifacts land under:

- `build/hw/same70/examples/blink/`
- `build/hw/same70/examples/uart_logger/`
- `build/hw/same70/examples/watchdog_probe/`
- `build/hw/same70/examples/analog_probe/`
- `build/hw/same70/examples/rtc_probe/`
- `build/hw/same70/examples/timer_pwm_probe/`
- `build/hw/same70/examples/i2c_scan/`
- `build/hw/same70/examples/spi_probe/`
- `build/hw/same70/examples/dma_probe/`
- `build/hw/same70/examples/can_probe/`

Run the suite in order. Stop on the first hard-fault/reset-loop.

## Stage 1: Bring-Up

### `blink`

- flash `build/hw/same70/examples/blink/blink.elf` or the generated `.bin/.hex`
- acceptance:
  - onboard LED starts visible 1 Hz blinking
  - repeated resets reproduce the same behavior

### `uart_logger`

- flash `build/hw/same70/examples/uart_logger/uart_logger.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `uart logger ready`
  - repeated `heartbeat loop=<n>` lines about once per second
  - LED continues toggling while logging runs

## Stage 2: Safe Board Services

### `watchdog_probe`

- flash `build/hw/same70/examples/watchdog_probe/watchdog_probe.elf`
- acceptance:
  - board keeps blinking at 2 Hz
  - no reset-loop while periodic refresh runs
  - probe remains stable over repeated manual resets

### `rtc_probe`

- flash `build/hw/same70/examples/rtc_probe/rtc_probe.elf`
- acceptance:
  - board boots and keeps blinking
  - no hard-fault/reset-loop after RTC configure path

## Stage 3: Timing And Analog

### `timer_pwm_probe`

- flash `build/hw/same70/examples/timer_pwm_probe/timer_pwm_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `timer/pwm probe ready`
  - period/duty logs appear once
  - board keeps running without reset-loop

### `analog_probe`

- flash `build/hw/same70/examples/analog_probe/analog_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `analog probe ready`
  - `dac active`
  - board keeps running without reset-loop

## Stage 4: Buses

### `i2c_scan`

- flash `build/hw/same70/examples/i2c_scan/i2c_scan.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `i2c scan ready`
  - scan completes and board remains responsive

### `spi_probe`

- flash `build/hw/same70/examples/spi_probe/spi_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `spi probe ready`
  - transfer log appears once
  - board remains responsive

## Stage 5: DMA

### `dma_probe`

- flash `build/hw/same70/examples/dma_probe/dma_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `dma probe ready`
  - one `debug-uart-tx binding=...` line
  - one `debug-uart-rx binding=...` line
  - board does not hard-fault or reset-loop after DMA setup

## Stage 6: CAN

### `can_probe`

- flash `build/hw/same70/examples/can_probe/can_probe.elf`
- acceptance:
  - board boots and keeps blinking
  - no hard-fault/reset-loop after CAN configure path

## Escalation Path

- `blink` failure:
  - compare against `same70-runtime-validation` and the SAME70 host MMIO bring-up tests
- `uart_logger` failure:
  - inspect debug-UART connector bindings in [board_uart.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_uart.hpp)
- `watchdog_probe` failure:
  - inspect [board_watchdog.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_watchdog.hpp) and [src/hal/watchdog.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/watchdog.hpp)
- `analog_probe` failure:
  - inspect [board_analog.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_analog.hpp)
- `timer_pwm_probe` failure:
  - inspect [src/hal/timer.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/timer.hpp) and [src/hal/pwm.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/pwm.hpp)
- `dma_probe` failure:
  - inspect typed DMA helpers in [board_dma.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_dma.hpp)
- `can_probe` failure:
  - inspect [src/hal/can.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/src/hal/can.hpp)

Use [CHECKLIST.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tests/hardware/same70/CHECKLIST.md) to record the run.
