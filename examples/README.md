# Alloy Examples

Os exemplos oficiais agora seguem o caminho runtime-driven:

- `board::init()`
- `board::make_*()`
- HAL pública única em `src/hal/*`

Exemplos principais:

- `blink`: LED e board abstraction básica
- `uart_logger`: logger com UART de debug
- `i2c_scan`: configuração de `I2C` e varredura de endereços
- `spi_probe`: configuração de `SPI` e transferência simples
- `dma_probe`: seleção tipada de bindings `DMA` no caminho oficial de board/runtime

Disponibilidade atual por perfil de board:

- `blink`: qualquer board descriptor-driven com `board.hpp`
- `uart_logger`: boards que expõem `board::make_debug_uart()`
- `i2c_scan` e `spi_probe`: boards descriptor-driven com flash suficiente para os exemplos
  completos e com helpers `board::make_i2c()`/`board::make_spi()`; hoje isso cobre
  `same70_xpld`
- `dma_probe`: boards com `board::make_debug_uart_*_dma()`; hoje isso cobre
  `same70_xpld` e `nucleo_f401re` no nível de binding tipado
