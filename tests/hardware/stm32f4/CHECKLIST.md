# STM32F4 Hardware Validation Checklist

Board:

- `nucleo_f401re`

Build:

```bash
python3 scripts/alloyctl.py bundle --board nucleo_f401re -j8
```

Recommended flash flow:

```bash
python3 scripts/alloyctl.py recover --board nucleo_f401re --target blink -j8
python3 scripts/alloyctl.py monitor --board nucleo_f401re
```

Then continue with:

```bash
python3 scripts/alloyctl.py flash --board nucleo_f401re --target uart_logger --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_f401re --target watchdog_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_f401re --target rtc_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_f401re --target timer_pwm_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_f401re --target analog_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_f401re --target dma_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_f401re --target time_probe --build-first -j8
```

## Stage 1: Bring-Up

| Check | Result | Notes |
|---|---|---|
| `blink` | ☐ pass / ☐ fail | LED visible, no stuck-on/stuck-off state |
| `uart_logger` | ☐ pass / ☐ fail | `uart logger ready` + heartbeat on VCOM |

## Stage 2: Safe Board Services

| Check | Result | Notes |
|---|---|---|
| `watchdog_probe` | ☐ pass / ☐ fail | No reset loop after watchdog configure/refresh |
| `rtc_probe` | ☐ pass / ☐ fail | Board stays alive after RTC configure |

## Stage 3: Timing And Analog

| Check | Result | Notes |
|---|---|---|
| `timer_pwm_probe` | ☐ pass / ☐ fail | Timer/PWM bring-up + observable loop |
| `analog_probe` | ☐ pass / ☐ fail | ADC/DAC configure path stable |
| `time_probe` | ☐ pass / ☐ fail | `time loop=... uptime_ms=...` prints cleanly |

## Stage 4: DMA

| Check | Result | Notes |
|---|---|---|
| `dma_probe` | ☐ pass / ☐ fail | DMA binding/config stable on hardware |

## Evidence

- hardware date:
- operator:
- flash flow used:
- monitor command used:
- serial port used:
- recovery required: `yes/no`
- recovery backend: `stm32cube/openocd/n/a`

## Summary

- overall status:
- follow-up issues:
