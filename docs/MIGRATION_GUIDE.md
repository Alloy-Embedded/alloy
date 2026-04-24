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

That rule also applies to the in-repo examples: board-oriented examples continue to teach
`board::init()` plus `board::make_*()`, while direct `uart::route` / `spi::route` / `i2c::route`
spelling is reserved for raw HAL documentation and isolated expert probes.

## modm -> Alloy

Map the concepts like this:

| modm | Alloy |
| --- | --- |
| `Board::initialize()` | `board::init()` |
| `GpioConnector<...>` | `alloy::hal::uart::route<...>` / `spi::route<...>` / `i2c::route<...>` |
| `UartHal`, `SpiMaster`, `I2cMaster` | typed `port_handle<Connector>` |
| board clock policy | generated `system_clock` / `clock_config` profiles |

Main difference:

- `alloy` keeps generated hardware facts in `src/device/**`
- the runtime API stays in `src/hal/**`
- public code may use concise aliases like `alloy::dev::periph::*`,
  `alloy::dev::pin::*`, and `alloy::dev::sig::*`, while `alloy::device::*`
  remains the canonical internal contract layer
- diagnostics and support claims are published through `alloyctl explain`, `alloyctl diff`, and [SUPPORT_MATRIX.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/SUPPORT_MATRIX.md)

Ergonomic direct HAL example:

```cpp
using DebugUart = alloy::hal::uart::route<
    alloy::dev::periph::USART2,
    alloy::hal::tx<alloy::dev::pin::PA2>,
    alloy::hal::rx<alloy::dev::pin::PA3>>;
```

For handwritten application code, prefer local namespace aliases so the direct HAL syntax stays
close to the shape users expect from modm:

```cpp
namespace hal = alloy::hal;
namespace dev = alloy::dev;

using DebugUart = hal::uart::route<
    dev::periph::USART2,
    hal::tx<dev::pin::PA2>,
    hal::rx<dev::pin::PA3>>;
```

If a board only publishes an expert signal spelling, keep the public route form but add the second template argument explicitly:

```cpp
using DebugUart = alloy::hal::uart::route<
    alloy::dev::periph::USART1,
    alloy::hal::tx<alloy::dev::pin::PB4, alloy::dev::sig::signal_txd1>,
    alloy::hal::rx<alloy::dev::pin::PA21, alloy::dev::sig::signal_rxd1>>;
```

## Register-level code or libopencm3 -> Alloy

Move in this order:

1. replace handwritten board constants with the board helper in `boards/**`
2. replace raw register setup with the typed HAL handle for that peripheral
3. keep direct register pokes only where there is no public HAL surface yet
4. use `alloyctl explain --board ...` to confirm the official clock and connector path

`alloyctl explain --board <board> --connector <alias>` now prints the ergonomic route spelling first and the canonical runtime/device form second, so migration work can stay on the short public syntax without losing expert traceability.

When moving code between two supported boards, use `alloyctl diff --from <source-board> --to <target-board>` first. That output is the maintained migration checklist for board-visible differences such as clock profile, debug UART path, connector aliases, required examples, and release gates.

If you still need raw bring-up, keep it isolated in one probe example and do not make it the public path.

Inside this repo, that means examples such as `uart_path_probe` and `manual_uart_probe` stay
isolated as expert bring-up probes, while the canonical examples keep the board helper path.
