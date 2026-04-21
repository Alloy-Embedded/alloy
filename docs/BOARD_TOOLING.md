# Board Tooling

Use one entrypoint:

```bash
python3 scripts/alloyctl.py <command> ...
```

Supported boards:

- `same70_xplained`
- `nucleo_g071rb`
- `nucleo_f401re`

## Build

```bash
python3 scripts/alloyctl.py build --board same70_xplained --target uart_logger
python3 scripts/alloyctl.py build --board nucleo_g071rb --target analog_probe
```

## Build full hardware bundle

```bash
python3 scripts/alloyctl.py bundle --board same70_xplained
python3 scripts/alloyctl.py bundle --board nucleo_g071rb
python3 scripts/alloyctl.py bundle --board nucleo_f401re
```

## Flash

```bash
python3 scripts/alloyctl.py flash --board same70_xplained --target uart_logger --build-first
python3 scripts/alloyctl.py flash --board nucleo_f401re --target dma_probe --build-first
```

Current flash backend:

- SAME70: `openocd -f board/atmel_same70_xplained.cfg`
- STM32G0: `openocd -f interface/stlink.cfg -f target/stm32g0x.cfg`
- STM32F4: `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg`

## UART monitor

```bash
python3 scripts/alloyctl.py monitor --board same70_xplained
python3 scripts/alloyctl.py monitor --board nucleo_f401re --port /dev/cu.usbmodem1234
```

## GDB server

```bash
python3 scripts/alloyctl.py gdbserver --board same70_xplained
python3 scripts/alloyctl.py gdbserver --board nucleo_g071rb --gdb-port 3333
```
