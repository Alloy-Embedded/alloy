## 1. Host UART Implementation

- [x] 1.1 Create `src/hal/host/uart.hpp` header
- [x] 1.2 Create `src/hal/host/uart.cpp` implementation
- [x] 1.3 Implement UartDevice concept
- [x] 1.4 Implement read_byte() using std::cin (non-blocking with select())
- [x] 1.5 Implement write_byte() using std::cout
- [x] 1.6 Implement available() using select() with zero timeout
- [x] 1.7 Implement configure() (validates config, stores for host)

## 2. Build Configuration

- [x] 2.1 Update `src/hal/host/CMakeLists.txt` to include UART
- [x] 2.2 Link with core error handling

## 3. Testing

- [x] 3.1 Create unit tests for host UART (test_uart_host.cpp)
- [x] 3.2 Test write operations (12 comprehensive tests)
- [x] 3.3 Test configuration validation
- [x] 3.4 Test BaudRate type safety
- [x] 3.5 Test UartConfig with defaults and custom values
- [x] 3.6 Verify UartDevice concept satisfaction
