# STM32G0 Hardware Validation Checklist

Board:

- `nucleo_g071rb`

Build:

```bash
cmake -S . -B build/hw/g071 \
  -DALLOY_BOARD=nucleo_g071rb \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/g071 --target stm32g0_hardware_validation_bundle --parallel 8
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

## Summary

- overall status:
- probe/flash flow used:
- serial port used:
- follow-up issues:
