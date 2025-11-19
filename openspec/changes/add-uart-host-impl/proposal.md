## Why

Need host UART implementation using stdin/stdout for testing and debugging without hardware.

## What Changes

- Create UART host implementation in `src/hal/host/uart.{hpp,cpp}`
- Use std::cin for read operations (non-blocking)
- Use std::cout for write operations
- Implement available() checking std::cin.rdbuf()->in_avail()
- Support basic configuration (baud rate is no-op on host)

## Impact

- Affected specs: hal-uart-host (new capability)
- Affected code: src/hal/host/uart.{hpp,cpp}
- Enables uart_echo example to run on host
