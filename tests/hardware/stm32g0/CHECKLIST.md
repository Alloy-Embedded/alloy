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
| `blink` | ☑ pass / ☐ fail | Recovery flow succeeded and LED blink is visible on hardware |
| `uart_logger` | ☑ pass / ☐ fail | VCOM path alive on hardware |

## Stage 2: Safe Board Services

| Check | Result | Notes |
|---|---|---|
| `watchdog_probe` | ☑ pass / ☐ fail | Hardware run passed |
| `rtc_probe` | ☑ pass / ☐ fail | Hardware run passed |

## Stage 3: Timing And Analog

| Check | Result | Notes |
|---|---|---|
| `timer_pwm_probe` | ☑ pass / ☐ fail | Hardware run passed |
| `analog_probe` | ☑ pass / ☐ fail | Hardware run passed |
| `time_probe` | ☑ pass / ☐ fail | Hardware run passed |

## Evidence

- hardware date:
- operator:
- flash flow used:
- monitor command used:
- serial port used:
- recovery required: `yes`
- recovery backend: `stm32cube`

## Summary

- overall status: stage 1 through stage 3 passed on hardware
- follow-up issues: none in the published default clock path; generated contract now selects `safe_hsi16` as the validated default bring-up profile
