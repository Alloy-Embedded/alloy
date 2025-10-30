## 1. UART Interface

- [ ] 1.1 Create `src/hal/interface/uart.hpp`
- [ ] 1.2 Define UartConfig struct (baud rate, data bits, parity, stop bits)
- [ ] 1.3 Define UartDevice concept with required operations
- [ ] 1.4 Add read_byte() returning Result<uint8_t, ErrorCode>
- [ ] 1.5 Add write_byte(uint8_t) returning Result<void, ErrorCode>
- [ ] 1.6 Add available() returning size_t
- [ ] 1.7 Add configure(UartConfig) method

## 2. Type Safety

- [ ] 2.1 Create BaudRate type in `src/core/units.hpp`
- [ ] 2.2 Add literals like 9600_baud, 115200_baud
- [ ] 2.3 Add compile-time validation for common baud rates
