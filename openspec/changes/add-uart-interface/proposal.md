## Why

UART is essential for debugging and communication. Need platform-agnostic UART interface using C++20 concepts.

## What Changes

- Create UART concepts in `src/hal/interface/uart.hpp`
- Define UartDevice concept with read(), write(), available() operations
- Define BaudRate type-safe wrapper using units
- Add configuration parameters (data bits, parity, stop bits)

## Impact

- Affected specs: hal-uart-interface (new capability)
- Affected code: src/hal/interface/uart.hpp
- Foundation for UART implementations (host uses stdin/stdout)
