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

Supported repo flow:

- `python3 scripts/alloyctl.py bundle --board nucleo_g071rb -j8`
- `python3 scripts/alloyctl.py flash --board nucleo_g071rb --target <example>`
- `python3 scripts/alloyctl.py recover --board nucleo_g071rb --target <example>` if the board is trapped by bad firmware
- `python3 scripts/alloyctl.py monitor --board nucleo_g071rb`

## Configure And Build

```bash
python3 scripts/alloyctl.py bundle --board nucleo_g071rb -j8
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

- flash `python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink`
- acceptance:
  - LD4 begins visible 1 Hz blinking shortly after reset

### `uart_logger`

- flash `python3 scripts/alloyctl.py flash --board nucleo_g071rb --target uart_logger`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `uart logger ready`
  - repeated `heartbeat loop=<n>` lines about once per second
  - LED keeps toggling while logging runs

## Stage 2: Safe Board Services

### `watchdog_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_g071rb --target watchdog_probe`
- acceptance:
  - board keeps blinking at 2 Hz
  - no reset-loop while the refresh loop runs
  - repeated manual resets reproduce the same behavior

### `rtc_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_g071rb --target rtc_probe`
- acceptance:
  - board boots and keeps blinking
  - no hard-fault/reset-loop after RTC configure path

## Stage 3: Timing And Analog

### `timer_pwm_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_g071rb --target timer_pwm_probe`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `timer/pwm probe ready`
  - period/duty logs appear once
  - board keeps running without reset-loop

### `analog_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_g071rb --target analog_probe`
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
