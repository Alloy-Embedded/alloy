# DMA Probe

Exemplo canônico do caminho público de `DMA` no runtime novo.

Ele não executa uma transferência real ainda. O objetivo deste exemplo é validar
o contrato tipado publicado por `alloy-devices` e o consumo oficial via:

- `board::init()`
- `board::make_debug_uart()`
- `board::make_debug_uart_tx_dma()`
- `board::make_debug_uart_rx_dma()`
- `uart.configure_tx_dma(...)`
- `uart.configure_rx_dma(...)`

Na UART de debug o exemplo imprime os ids tipados do binding DMA selecionado
para TX e RX do UART de debug da board, além dos endereços de registrador
usados no path público do periférico.

As direções usadas no exemplo são explícitas:

- TX: `memory_to_peripheral`
- RX: `peripheral_to_memory`
