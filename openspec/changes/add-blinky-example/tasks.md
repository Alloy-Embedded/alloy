## 1. Blinky Implementation

- [x] 1.1 Create `examples/blinky/` directory
- [x] 1.2 Create `examples/blinky/main.cpp`
- [x] 1.3 Include GPIO interface headers
- [x] 1.4 Instantiate ConfiguredGpioPin<25, PinMode::Output> (uses host::GpioPin<25>)
- [x] 1.5 Implement infinite loop with toggle() and delay
- [x] 1.6 Create `examples/blinky/CMakeLists.txt`

## 2. Platform Utilities

- [x] 2.1 Create `src/platform/delay.hpp` for delay_ms()
- [x] 2.2 Implement delay_ms() for host using std::this_thread::sleep_for()

## 3. Build and Test

- [x] 3.1 Configure CMake for host target
- [x] 3.2 Build blinky example
- [x] 3.3 Run and verify console output shows toggling
- [x] 3.4 Document build/run instructions in examples/blinky/README.md
