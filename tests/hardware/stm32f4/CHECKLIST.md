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
| `blink` | ☑ pass / ☐ fail | Hardware run passed |
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

## Stage 4: DMA

| Check | Result | Notes |
|---|---|---|
| `dma_probe` | ☐ pass / ☐ fail | Hardware gap was traced to missing published DMA stream IRQ descriptors; codegen + host-MMIO are fixed, hardware rerun still pending |

## Evidence

- hardware date:
- operator:
- flash flow used:
- monitor command used:
- serial port used:
- recovery required: `yes/no`
- recovery backend: `stm32cube/openocd/n/a`

## Summary

- overall status: stage 1 through stage 3 passed on hardware; DMA publish path fixed and pending hardware rerun
- follow-up issues: rerun `dma_probe` on hardware against the republished `alloy-devices` contract to close the last STM32F4 board gap
