## Why

Need host (PC) GPIO implementation for development and testing without hardware. Must print GPIO operations to console.

## What Changes

- Create GPIO host mock in `src/hal/host/gpio.hpp` and `.cpp`
- Implement GpioPin concept for host platform
- Print GPIO operations to stdout (e.g., "[GPIO Mock] Pin 25 set HIGH")
- Maintain pin state internally for read() operations
- Support CMakeLists.txt for building host HAL

## Impact

- Affected specs: hal-gpio-host (new capability)
- Affected code: src/hal/host/gpio.{hpp,cpp}
- Enables blinky example to run on host
