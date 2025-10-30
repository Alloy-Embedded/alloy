## Why

Support for Renesas RL78 (CF_RL78) MCU family - a 16-bit low-power MCU that the project is currently using. RL78 has different architecture from ARM Cortex-M (different registers, instruction set, toolchain).

## What Changes

- Create RL78 HAL implementation in `src/hal/rl78/`
- Implement GPIO for RL78 (port-based, not individual pins)
- Implement UART for RL78 (SAU - Serial Array Unit)
- Add RL78 toolchain support (CC-RL or GCC for RL78)
- Create board definition for CF_RL78
- Integrate with CMSIS-like headers from Renesas

## Impact

- Affected specs: hal-rl78 (new capability)
- Affected code: src/hal/rl78/, cmake/toolchains/rl78-gcc.cmake
- **BREAKING**: First non-ARM architecture, validates cross-platform design
- Validates that GPIO/UART interfaces work for 16-bit MCUs
