## 1. Host UART Implementation

- [ ] 1.1 Create `src/hal/host/uart.hpp` header
- [ ] 1.2 Create `src/hal/host/uart.cpp` implementation
- [ ] 1.3 Implement UartDevice concept
- [ ] 1.4 Implement read_byte() using std::cin (non-blocking)
- [ ] 1.5 Implement write_byte() using std::cout
- [ ] 1.6 Implement available() using in_avail()
- [ ] 1.7 Implement configure() (no-op for host, just validate)

## 2. Build Configuration

- [ ] 2.1 Update `src/hal/host/CMakeLists.txt` to include UART
- [ ] 2.2 Link with core error handling

## 3. Testing

- [ ] 3.1 Create unit tests for host UART
- [ ] 3.2 Test write operations
- [ ] 3.3 Test available() with mock input
