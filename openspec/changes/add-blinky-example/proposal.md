## Why

Need a simple example (blinky) to validate the entire stack: GPIO interface, host implementation, and CMake build system.

## What Changes

- Create `examples/blinky/` directory
- Implement main.cpp using GPIO interface
- Create CMakeLists.txt for blinky example
- Add delay function for host platform
- Document how to build and run

## Impact

- Affected specs: examples-blinky (new capability)
- Affected code: examples/blinky/main.cpp
- First end-to-end validation of the architecture
