# SAME70 Hardware Spot-Check

Representative board:

- `same70_xplained`

## Why This Board

- first Renode family already validated in Alloy
- exposes debug UART through the board support layer
- exposes typed DMA helpers through the board support layer

## Required Equipment

- SAME70 Xplained Ultra board
- board power/debug connection through the on-board debugger used by the lab
- serial terminal attached to the board debug UART path

The repo does not hardcode a flashing tool here. Use the on-board debugger or external probe flow
already used for SAME70 boards in the lab.

## Configure And Build

```bash
cmake -S . -B build/hw/same70 \
  -DALLOY_BOARD=same70_xplained \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/same70 --target blink uart_logger dma_probe --parallel 8
```

Artifacts land under:

- `build/hw/same70/examples/blink/`
- `build/hw/same70/examples/uart_logger/`
- `build/hw/same70/examples/dma_probe/`

## Mandatory Checks

### `blink`

- flash `build/hw/same70/examples/blink/blink.elf` or the generated `.bin/.hex`
- acceptance:
  - onboard LED starts visible 1 Hz blinking
  - repeated resets reproduce the same behavior

### `uart_logger`

- flash `build/hw/same70/examples/uart_logger/uart_logger.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `uart logger ready`
  - repeated `heartbeat loop=<n>` lines about once per second
  - LED continues toggling while logging runs

## Extended Check

### `dma_probe`

- flash `build/hw/same70/examples/dma_probe/dma_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `dma probe ready`
  - one `debug-uart-tx binding=...` line
  - one `debug-uart-rx binding=...` line
  - board does not hard-fault or reset-loop after DMA setup

## Escalation Path

- `blink` failure:
  - compare against `same70-runtime-validation` and the SAME70 host MMIO bring-up tests
- `uart_logger` failure:
  - inspect debug-UART connector bindings in [board_uart.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_uart.hpp)
- `dma_probe` failure:
  - inspect typed DMA helpers in [board_dma.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_dma.hpp)
