# Alloy Migration Guide

## STM32Cube HAL/LL -> Alloy

Map the concepts like this:

| STM32Cube | Alloy |
| --- | --- |
| `MX_*` board setup | `boards/**` helpers + `board::make_*()` |
| `HAL_UART_Init()` | `alloy::hal::uart::port_handle<...>.configure()` |
| `HAL_SPI_Init()` | `alloy::hal::spi::port_handle<...>.configure()` |
| `HAL_I2C_Init()` | `alloy::hal::i2c::port_handle<...>.configure()` |
| `HAL_DMA_Init()` | `configure_tx_dma()` / `configure_rx_dma()` on typed handles |
| `HAL_RCC_*` clock setup | `alloy::device::system_clock::*` or `alloy::device::clock_config::*` |

Start from a board helper, not from raw peripheral numbers.

## modm -> Alloy

Map the concepts like this:

| modm | Alloy |
| --- | --- |
| `Board::initialize()` | `board::init()` |
| `GpioConnector<...>` | `alloy::hal::connection::connector<...>` |
| `UartHal`, `SpiMaster`, `I2cMaster` | typed `port_handle<Connector>` |
| board clock policy | generated `system_clock` / `clock_config` profiles |

Main difference:

- `alloy` keeps generated hardware facts in `src/device/**`
- the runtime API stays in `src/hal/**`
- diagnostics and support claims are published through `alloyctl explain`, `alloyctl diff`, and [SUPPORT_MATRIX.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md)

## Register-level code or libopencm3 -> Alloy

Move in this order:

1. replace handwritten board constants with the board helper in `boards/**`
2. replace raw register setup with the typed HAL handle for that peripheral
3. keep direct register pokes only where there is no public HAL surface yet
4. use `alloyctl explain --board ...` to confirm the official clock and connector path

If you still need raw bring-up, keep it isolated in one probe example and do not make it the public path.
