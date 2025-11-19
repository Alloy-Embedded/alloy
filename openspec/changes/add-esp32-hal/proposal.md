## Why

Support for ESP32 - a popular WiFi/Bluetooth capable MCU from Espressif. ESP32 uses Xtensa LX6 (or RISC-V in newer variants) architecture, very different from ARM and RL78.

## What Changes

- Create ESP32 HAL implementation in `src/hal/esp32/`
- Implement GPIO for ESP32 (using GPIO matrix)
- Implement UART for ESP32 (UART0/1/2)
- Add ESP32 toolchain support (ESP-IDF or Arduino)
- Create board definition for ESP32-DevKitC
- Handle ESP32 specifics (dual-core, WiFi, FreeRTOS integration)

## Impact

- Affected specs: hal-esp32 (new capability)  
- Affected code: src/hal/esp32/, cmake/toolchains/esp32.cmake
- **BREAKING**: First non-ARM, non-16bit architecture
- **BREAKING**: ESP32 typically requires FreeRTOS (different from bare-metal)
- Validates architecture flexibility across radically different platforms
