# Nucleo-G071RB

Supported board on the active runtime path.

## Current Status

- MCU: `STM32G071RBT6`
- official clock profile: `HSI + PLL, 64 MHz`
- onboard LED: `PA5` / LD4 active high
- user button: `PC13` / B1 active low
- debug UART: `USART2` on `PA2`/`PA3` through ST-LINK VCP at `115200 8N1`
- public board helpers:
  - `board::init()`
  - `board::led::{init,on,off,toggle}`
  - `board::make_debug_uart()`
  - `board::make_adc()`
  - `board::make_dac()`
  - `board::make_watchdog()`

## Official Headers

- [board.hpp](board.hpp)
- [board_uart.hpp](board_uart.hpp)
- [board_analog.hpp](board_analog.hpp)
- [board_watchdog.hpp](board_watchdog.hpp)

## Supported Tooling Flow

```bash
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink --build-first
python3 scripts/alloyctl.py monitor --board nucleo_g071rb
python3 scripts/alloyctl.py validate --board nucleo_g071rb
```

## Canonical Examples

- [blink](../../examples/blink)
- [time_probe](../../examples/time_probe)
- [uart_logger](../../examples/uart_logger)
- [analog_probe](../../examples/analog_probe)
- [watchdog_probe](../../examples/watchdog_probe)

## Known Limits

- this board is foundational for the runtime path, but not every public HAL class has a board helper yet
- `DMA`, `SPI`, and `CAN` are not exposed here as first-class board helpers today