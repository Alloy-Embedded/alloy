## 1. Host GPIO Implementation

- [x] 1.1 Create `src/hal/host/gpio.hpp` header
- [x] 1.2 Create `src/hal/host/gpio.cpp` implementation
- [x] 1.3 Implement GpioPin template class
- [x] 1.4 Add internal state tracking (bool state_)
- [x] 1.5 Implement set_high() with console output
- [x] 1.6 Implement set_low() with console output
- [x] 1.7 Implement toggle() with console output
- [x] 1.8 Implement read() returning internal state
- [x] 1.9 Implement configure() with mode tracking

## 2. Build Configuration

- [x] 2.1 Create `src/hal/host/CMakeLists.txt`
- [x] 2.2 Add library target for host GPIO
- [x] 2.3 Link with core error handling (interface headers)

## 3. Testing

- [ ] 3.1 Create unit tests for host GPIO (deferred - no test infrastructure yet)
- [ ] 3.2 Test set_high() changes state (deferred)
- [ ] 3.3 Test toggle() flips state (deferred)
- [ ] 3.4 Verify console output (manual check - will be done in blinky example)
