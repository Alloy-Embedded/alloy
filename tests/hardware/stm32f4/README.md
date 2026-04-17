# STM32F4 Hardware Spot-Check

Representative board:

- `nucleo_f401re`

## Why This Board

- selected next Renode family after SAME70
- exposes debug UART and typed DMA helpers through the board layer
- gives the validation ladder one representative Cortex-M4F silicon target

## Required Equipment

- Nucleo-F401RE board
- ST-LINK USB connection for flashing/debug
- serial terminal attached to the ST-LINK virtual COM port

The repo currently stops at build artifacts. Flash with the ST-LINK/OpenOCD flow already used in
the lab.

## Configure And Build

```bash
cmake -S . -B build/hw/f401 \
  -DALLOY_BOARD=nucleo_f401re \
  -DALLOY_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build/hw/f401 --target blink uart_logger dma_probe --parallel 8
```

Artifacts land under:

- `build/hw/f401/examples/blink/`
- `build/hw/f401/examples/uart_logger/`
- `build/hw/f401/examples/dma_probe/`

## Mandatory Checks

### `blink`

- flash `build/hw/f401/examples/blink/blink.elf`
- acceptance:
  - onboard LED begins visible 1 Hz blinking shortly after reset

### `uart_logger`

- flash `build/hw/f401/examples/uart_logger/uart_logger.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `uart logger ready`
  - repeated `heartbeat loop=<n>` lines about once per second
  - LED keeps toggling while logging runs

## Extended Check

### `dma_probe`

- flash `build/hw/f401/examples/dma_probe/dma_probe.elf`
- serial settings:
  - `115200 8N1`
- acceptance:
  - `dma probe ready`
  - one TX binding line
  - one RX binding line
  - board keeps running without reset-loop or obvious hard-fault

## Notes

- this is the first ST family where both future Renode coverage and current hardware DMA coverage
  can meet on the same representative board
- failures here should be compared against the future `stm32f4` Renode bring-up once it lands
