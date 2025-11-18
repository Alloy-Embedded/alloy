## Why

Need UART echo example to validate UART interface and host implementation.

## What Changes

- Create `examples/uart_echo/` directory
- Implement main.cpp that echoes stdin to stdout
- Use UART interface (not direct stdin/stdout)
- Demonstrate error handling with Result<T, ErrorCode>
- Document build/run instructions

## Impact

- Affected specs: examples-uart-echo (new capability)
- Affected code: examples/uart_echo/main.cpp
- Validates UART stack end-to-end
