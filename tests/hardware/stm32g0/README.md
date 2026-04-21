# STM32G0 Hardware Validation Bundle

Representative board:

- `nucleo_g071rb`

## Why This Board

- current ST foundation board already covered in host MMIO
- exposes debug UART through the board support layer
- small, fast board for validating the public runtime path on real silicon
- covers the first STM32 family with ADC, RTC, timer, PWM, and watchdog examples in the active path

## Required Equipment

- Nucleo-G071RB board
- ST-LINK USB connection for flashing/debug
- serial terminal attached to the ST-LINK virtual COM port

The repo does not hardcode a flashing tool here. Use the ST-LINK/OpenOCD flow already used in the
lab.

## Configure And Build

```bash
cmake -S . -B build/hw/g071 \
  -DALLOY_BOARD=nucleo_g071rb \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/g071 --target stm32g0_hardware_validation_bundle --parallel 8
```

Artifacts land under:

- `build/hw/g071/examples/blink/`
- `build/hw/g071/examples/uart_logger/`
- `build/hw/g071/examples/watchdog_probe/`
- `build/hw/g071/examples/analog_probe/`
- `build/hw/g071/examples/rtc_probe/`
- `build/hw/g071/examples/timer_pwm_probe/`

Run the suite in order. Stop on the first hard-fault/reset-loop.

## Stage 1: Bring-Up

### `blink`

- flash `build/hw/g071/examples/blink/blink.elf`
- acceptance:
  - LD4 begins visible 1 Hz blinking shortly after reset

### `uart_logger`

- flash `build/hw/g071/examples/uart_logger/uart_logger.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `uart logger ready`
  - repeated `heartbeat loop=<n>` lines about once per second
  - LED keeps toggling while logging runs

## Stage 2: Safe Board Services

### `watchdog_probe`

- flash `build/hw/g071/examples/watchdog_probe/watchdog_probe.elf`
- acceptance:
  - board keeps blinking at 2 Hz
  - no reset-loop while the refresh loop runs
  - repeated manual resets reproduce the same behavior

### `rtc_probe`

- flash `build/hw/g071/examples/rtc_probe/rtc_probe.elf`
- acceptance:
  - board boots and keeps blinking
  - no hard-fault/reset-loop after RTC configure path

## Stage 3: Timing And Analog

### `timer_pwm_probe`

- flash `build/hw/g071/examples/timer_pwm_probe/timer_pwm_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `timer/pwm probe ready`
  - period/duty logs appear once
  - board keeps running without reset-loop

### `analog_probe`

- flash `build/hw/g071/examples/analog_probe/analog_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `analog probe ready`
  - `dac active`
  - board keeps running without reset-loop

## Notes

- `dma_probe` is intentionally not in the STM32G0 foundation hardware bundle yet because the board
  layer still does not expose typed debug-UART DMA helpers for this board.
- if this board passes on silicon while host MMIO and Renode also stay green, the STM32G0 runtime
  path has both broad and real-world confidence.

Use [CHECKLIST.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tests/hardware/stm32g0/CHECKLIST.md) to record the run.
