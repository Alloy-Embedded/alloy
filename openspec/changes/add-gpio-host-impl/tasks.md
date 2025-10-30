## 1. Host GPIO Implementation

- [ ] 1.1 Create `src/hal/host/gpio.hpp` header
- [ ] 1.2 Create `src/hal/host/gpio.cpp` implementation
- [ ] 1.3 Implement GpioPin template class
- [ ] 1.4 Add internal state tracking (bool state_)
- [ ] 1.5 Implement set_high() with console output
- [ ] 1.6 Implement set_low() with console output
- [ ] 1.7 Implement toggle() with console output
- [ ] 1.8 Implement read() returning internal state
- [ ] 1.9 Implement configure() with mode tracking

## 2. Build Configuration

- [ ] 2.1 Create `src/hal/host/CMakeLists.txt`
- [ ] 2.2 Add library target for host GPIO
- [ ] 2.3 Link with core error handling

## 3. Testing

- [ ] 3.1 Create unit tests for host GPIO
- [ ] 3.2 Test set_high() changes state
- [ ] 3.3 Test toggle() flips state
- [ ] 3.4 Verify console output (manual check)
