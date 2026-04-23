# Board Tooling

Use one entrypoint:

```bash
python3 scripts/alloyctl.py <command> ...
```

This is the supported board-oriented UX for:

- configure
- build
- bundle
- flash
- monitor
- gdbserver
- validation through workflow presets

Supported boards:

- `same70_xplained`
- `nucleo_g071rb`
- `nucleo_f401re`

Validation-only target:

- `host`

## Configure

```bash
python3 scripts/alloyctl.py configure --board same70_xplained
python3 scripts/alloyctl.py configure --board nucleo_g071rb --build-tests
```

`configure` creates or refreshes the board-specific build directory under `build/hw/...`.

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

`bundle` builds the board's declared hardware validation bundle target.

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

## Validation

```bash
python3 scripts/alloyctl.py validate --board host
python3 scripts/alloyctl.py validate --board same70_xplained
python3 scripts/alloyctl.py validate --board same70_xplained --kind zero-overhead
python3 scripts/alloyctl.py validate --board nucleo_g071rb --kind smoke
python3 scripts/alloyctl.py validate --board nucleo_f401re
```

Current validation mapping:

- `host` -> `host-mmio-validation`
- `same70_xplained` -> `same70-runtime-validation`, `same70-renode-smoke`, `same70-zero-overhead`
- `nucleo_g071rb` -> `stm32g0-runtime-validation`, `stm32g0-renode-smoke`
- `nucleo_f401re` -> `stm32f4-runtime-validation`, `stm32f4-renode-smoke`

For a shorter getting-started path, use [QUICKSTART.md](QUICKSTART.md).
