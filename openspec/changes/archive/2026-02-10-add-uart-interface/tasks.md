## 1. UART Interface

- [x] 1.1 Create `src/hal/interface/uart.hpp`
- [x] 1.2 Define UartConfig struct (baud rate, data bits, parity, stop bits)
- [x] 1.3 Define UartDevice concept with required operations
- [x] 1.4 Add read_byte() returning Result<uint8_t, ErrorCode>
- [x] 1.5 Add write_byte(uint8_t) returning Result<void, ErrorCode>
- [x] 1.6 Add available() returning size_t
- [x] 1.7 Add configure(UartConfig) method

## 2. Type Safety

- [x] 2.1 Create BaudRate type in `src/core/units.hpp`
- [x] 2.2 Add literals like 9600_baud, 115200_baud
- [x] 2.3 Add compile-time validation for common baud rates

## 3. Additional Features

- [x] 3.1 Add ConfiguredUart template wrapper
- [x] 3.2 Add write() method for byte arrays
- [x] 3.3 Add write_str() method for C strings
- [x] 3.4 Add DataBits, Parity, and StopBits enums
