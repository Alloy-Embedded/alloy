# STM32F4 Hardware Validation Bundle

Representative board:

- `nucleo_f401re`

## Why This Board

- selected next Renode family after SAME70
- exposes debug UART and typed DMA helpers through the board layer
- gives the validation ladder one representative Cortex-M4F silicon target
- covers runtime DMA and the public analog/timing/watchdog examples on a second vendor family

## Required Equipment

- Nucleo-F401RE board
- ST-LINK USB connection for flashing/debug
- serial terminal attached to the ST-LINK virtual COM port

Supported repo flow:

- `python3 scripts/alloyctl.py bundle --board nucleo_f401re -j8`
- `python3 scripts/alloyctl.py flash --board nucleo_f401re --target <example>`
- `python3 scripts/alloyctl.py recover --board nucleo_f401re --target <example>` if the board is trapped by bad firmware
- `python3 scripts/alloyctl.py monitor --board nucleo_f401re`

## Configure And Build

```bash
python3 scripts/alloyctl.py bundle --board nucleo_f401re -j8
```

Artifacts land under:

- `build/hw/f401/examples/blink/`
- `build/hw/f401/examples/uart_logger/`
- `build/hw/f401/examples/watchdog_probe/`
- `build/hw/f401/examples/analog_probe/`
- `build/hw/f401/examples/rtc_probe/`
- `build/hw/f401/examples/timer_pwm_probe/`
- `build/hw/f401/examples/dma_probe/`

Run the suite in order. Stop on the first hard-fault/reset-loop.

## Stage 1: Bring-Up

### `blink`

- flash `python3 scripts/alloyctl.py flash --board nucleo_f401re --target blink`
- acceptance:
  - onboard LED begins visible 1 Hz blinking shortly after reset

### `uart_logger`

- flash `python3 scripts/alloyctl.py flash --board nucleo_f401re --target uart_logger`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `uart logger ready`
  - repeated `heartbeat loop=<n>` lines about once per second
  - LED keeps toggling while logging runs

## Stage 2: Safe Board Services

### `watchdog_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_f401re --target watchdog_probe`
- acceptance:
  - board keeps blinking at 2 Hz
  - no reset-loop while the refresh loop runs
  - repeated manual resets reproduce the same behavior

### `rtc_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_f401re --target rtc_probe`
- acceptance:
  - board boots and keeps blinking
  - no hard-fault/reset-loop after RTC configure path

## Stage 3: Timing And Analog

### `timer_pwm_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_f401re --target timer_pwm_probe`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `timer/pwm probe ready`
  - period/duty logs appear once
  - board keeps running without reset-loop

### `analog_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_f401re --target analog_probe`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `analog probe ready`
  - board keeps running without reset-loop

## Stage 4: DMA

### `dma_probe`

- flash `python3 scripts/alloyctl.py flash --board nucleo_f401re --target dma_probe`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `dma probe ready`
  - one TX binding line
  - one RX binding line
  - board keeps running without reset-loop or obvious hard-fault

## Notes

- this is the first ST family where both future Renode coverage and current hardware DMA coverage
  can meet on the same representative board
- failures here should be compared against the future `stm32f4` Renode bring-up once it lands

Use [CHECKLIST.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tests/hardware/stm32f4/CHECKLIST.md) to record the run.
