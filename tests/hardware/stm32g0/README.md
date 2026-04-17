# STM32G0 Hardware Spot-Check

Representative board:

- `nucleo_g071rb`

## Why This Board

- current ST foundation board already covered in host MMIO
- exposes debug UART through the board support layer
- small, fast board for validating the minimal startup + GPIO + UART path on real silicon

## Required Equipment

- Nucleo-G071RB board
- ST-LINK USB connection for flashing/debug
- serial terminal attached to the ST-LINK virtual COM port

The repo currently documents the runbook only. Flash with the ST-LINK/OpenOCD flow already used in
the lab.

## Configure And Build

```bash
cmake -S . -B build/hw/g071 \
  -DALLOY_BOARD=nucleo_g071rb \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/g071 --target blink uart_logger --parallel 8
```

Artifacts land under:

- `build/hw/g071/examples/blink/`
- `build/hw/g071/examples/uart_logger/`

## Mandatory Checks

### `blink`

- flash `build/hw/g071/examples/blink/blink.elf`
- acceptance:
  - LD4 begins visible 1 Hz blinking shortly after reset

### `uart_logger`

- flash `build/hw/g071/examples/uart_logger/uart_logger.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `uart logger ready`
  - repeated `heartbeat loop=<n>` lines about once per second
  - LED keeps toggling while logging runs

## Notes

- `dma_probe` is intentionally not in the STM32G0 foundation runbook yet because the current board
  support layer does not expose typed debug-UART DMA helpers for this board.
- if this board passes on silicon while host MMIO also stays green, the ST descriptor path has both
  broad and real-world confidence for startup-adjacent flows.
