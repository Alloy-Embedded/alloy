## Why

Need platform-agnostic GPIO interface using C++20 concepts to define contracts for all GPIO implementations.

## What Changes

- Create GPIO concepts in `src/hal/interface/gpio.hpp`
- Define GpioPin concept with set_high(), set_low(), toggle(), read() operations
- Define PinMode enum (Input, Output, InputPullUp, InputPullDown)
- Create ConfiguredGpioPin template for compile-time pin configuration

## Impact

- Affected specs: hal-gpio-interface (new capability)
- Affected code: src/hal/interface/gpio.hpp
- Foundation for all GPIO implementations (host, rp2040, stm32f4)
