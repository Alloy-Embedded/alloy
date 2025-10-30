## 1. Blinky Implementation

- [ ] 1.1 Create `examples/blinky/` directory
- [ ] 1.2 Create `examples/blinky/main.cpp`
- [ ] 1.3 Include GPIO interface headers
- [ ] 1.4 Instantiate ConfiguredGpioPin<25, PinMode::Output>
- [ ] 1.5 Implement infinite loop with toggle() and delay
- [ ] 1.6 Create `examples/blinky/CMakeLists.txt`

## 2. Platform Utilities

- [ ] 2.1 Create `src/platform/delay.hpp` for delay_ms()
- [ ] 2.2 Implement delay_ms() for host using std::this_thread::sleep_for()

## 3. Build and Test

- [ ] 3.1 Configure CMake for host target
- [ ] 3.2 Build blinky example
- [ ] 3.3 Run and verify console output shows toggling
- [ ] 3.4 Document build/run instructions in examples/blinky/README.md
