# STM32G0 Hardware Validation Checklist

Board:

- `nucleo_g071rb`

Build:

```bash
python3 scripts/alloyctl.py bundle --board nucleo_g071rb -j8
```

Recommended flash flow:

```bash
python3 scripts/alloyctl.py recover --board nucleo_g071rb --target blink -j8
python3 scripts/alloyctl.py monitor --board nucleo_g071rb
```

Then continue with:

```bash
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target uart_logger --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target watchdog_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target rtc_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target timer_pwm_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target analog_probe --build-first -j8
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target time_probe --build-first -j8
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
