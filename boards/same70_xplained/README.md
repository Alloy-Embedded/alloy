# SAME70 Xplained

Suporte de board no caminho runtime atual.

## Estado Atual

- MCU: `ATSAME70Q21B`
- clock oficial atual: `12 MHz crystal + PLLA 150 MHz`
- LED onboard: `PC8` ativo em nível baixo
- UART de debug: `USART1` em `PB4`/`PA21` via EDBG VCOM, `115200 8N1`
- helpers públicos de board:
  - `board::init()`
  - `board::led::{init,on,off,toggle}`
  - `board::make_debug_uart()`
  - `board::make_i2c()`
  - `board::make_spi()`
  - `board::make_debug_uart_tx_dma()`
  - `board::make_debug_uart_rx_dma()`

## Headers Oficiais

- [board.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board.hpp)
- [board_uart.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_uart.hpp)
- [board_i2c.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_i2c.hpp)
- [board_spi.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_spi.hpp)
- [board_dma.hpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/same70_xplained/board_dma.hpp)

## Uso

```cpp
#include "boards/same70_xplained/board.hpp"
#include "boards/same70_xplained/board_uart.hpp"

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    uart.configure().unwrap();

    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    }
}
```

## Exemplos Canônicos

- [blink](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/blink)
- [uart_logger](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/uart_logger)
- [i2c_scan](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/i2c_scan)
- [spi_probe](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/spi_probe)
- [dma_probe](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/dma_probe)

## Limites Conhecidos

- `board::init()` libera `PB4` do bloco JTAG/System I/O para a VCOM da EDBG
- `DMA` continua publicado separadamente; valide o binding disponível antes de assumir que ele segue o mesmo caminho da UART de debug
