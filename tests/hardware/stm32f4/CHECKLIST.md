# STM32F4 Hardware Validation Checklist

Board:

- `nucleo_f401re`

Build:

```bash
cmake -S . -B build/hw/f401 \
  -DALLOY_BOARD=nucleo_f401re \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/f401 --target stm32f4_hardware_validation_bundle --parallel 8
```

## Stage 1: Bring-Up

| Check | Result | Notes |
|---|---|---|
| `blink` | ☐ pass / ☐ fail | |
| `uart_logger` | ☐ pass / ☐ fail | |

## Stage 2: Safe Board Services

| Check | Result | Notes |
|---|---|---|
| `watchdog_probe` | ☐ pass / ☐ fail | |
| `rtc_probe` | ☐ pass / ☐ fail | |

## Stage 3: Timing And Analog

| Check | Result | Notes |
|---|---|---|
| `timer_pwm_probe` | ☐ pass / ☐ fail | |
| `analog_probe` | ☐ pass / ☐ fail | |

## Stage 4: DMA

| Check | Result | Notes |
|---|---|---|
| `dma_probe` | ☐ pass / ☐ fail | |

## Summary

- overall status:
- probe/flash flow used:
- serial port used:
- follow-up issues:
