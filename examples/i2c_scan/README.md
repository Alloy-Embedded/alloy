# I2C Scan

Exemplo mínimo do caminho oficial de `I2C`:

- `board::init()`
- `board::make_debug_uart()`
- `board::make_i2c()`
- `bus.configure()`
- `bus.scan_bus(...)`

O exemplo não depende de um sensor específico. Ele apenas varre o barramento e imprime os
endereços encontrados na UART de debug da board selecionada.
