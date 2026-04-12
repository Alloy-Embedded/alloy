# SPI Probe

Exemplo mínimo do caminho oficial de `SPI`:

- `board::init()`
- `board::make_debug_uart()`
- `board::make_spi()`
- `bus.configure()`
- `bus.transfer(...)`

O exemplo envia um frame simples `0x9F 00 00 00` e imprime os bytes recebidos na UART de debug.
Sem um dispositivo conectado, o valor recebido depende do hardware externo.
