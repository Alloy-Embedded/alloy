# Nucleo-F401RE

Supported board on the active runtime path.

## Current Status

- MCU: `STM32F401RET6`
- official clock profile: `HSE + PLL, 84 MHz`
- onboard LED: `PA5` / LD2 active high
- user button: `PC13` / B1 active low
- debug UART: `USART2` on `PA2`/`PA3` through ST-LINK VCP at `115200 8N1`
- public board helpers:
  - `board::init()`
  - `board::led::{init,on,off,toggle}`
  - `board::make_debug_uart()`
  - `board::make_spi()`
  - `board::make_adc()`
  - `board::make_watchdog()`
  - `board::make_debug_uart_tx_dma()`
  - `board::make_debug_uart_rx_dma()`

## Official Headers

- [board.hpp](board.hpp)
- [board_uart.hpp](board_uart.hpp)
- [board_spi.hpp](board_spi.hpp)
- [board_dma.hpp](board_dma.hpp)
- [board_analog.hpp](board_analog.hpp)
- [board_watchdog.hpp](board_watchdog.hpp)

## Supported Tooling Flow

```bash
python3 scripts/alloyctl.py flash --board nucleo_f401re --target blink --build-first
python3 scripts/alloyctl.py monitor --board nucleo_f401re
python3 scripts/alloyctl.py validate --board nucleo_f401re
```

## Canonical Examples

- [blink](../../examples/blink)
- [time_probe](../../examples/time_probe)
- [uart_logger](../../examples/uart_logger)
- [dma_probe](../../examples/dma_probe)
- [analog_probe](../../examples/analog_probe)
- [watchdog_probe](../../examples/watchdog_probe)

## Known Limits

- `I2C`, `RTC`, and `CAN` are not exposed here as first-class board helpers today
- `DMA` support is intentionally scoped around the debug UART path exposed by `board_dma.hpp`