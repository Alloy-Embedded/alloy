## 1. ESP32 Toolchain Setup

- [ ] 1.1 Research ESP32 build options (ESP-IDF vs Arduino)
- [ ] 1.2 Decide: Use ESP-IDF or custom bare-metal approach
- [ ] 1.3 Create `cmake/toolchains/esp32.cmake`
- [ ] 1.4 Configure xtensa-esp32-elf-gcc or riscv32-esp-elf-gcc
- [ ] 1.5 Set ESP32-specific compiler flags

## 2. ESP32 Register Definitions

- [ ] 2.1 Obtain ESP32 register headers (from ESP-IDF)
- [ ] 2.2 Create wrapper in `src/platform/esp32/`
- [ ] 2.3 Define GPIO registers (GPIO_OUT_REG, GPIO_ENABLE_REG)
- [ ] 2.4 Define UART registers (UART_FIFO, UART_STATUS)

## 3. ESP32 GPIO Implementation

- [ ] 3.1 Create `src/hal/esp32/gpio.hpp`
- [ ] 3.2 Create `src/hal/esp32/gpio.cpp`
- [ ] 3.3 Implement GPIO using GPIO matrix (ESP32 specific)
- [ ] 3.4 Handle GPIO 34-39 (input only pins)
- [ ] 3.5 Implement set_high() using GPIO_OUT_W1TS_REG
- [ ] 3.6 Implement set_low() using GPIO_OUT_W1TC_REG
- [ ] 3.7 Implement toggle() using XOR
- [ ] 3.8 Implement read() using GPIO_IN_REG
- [ ] 3.9 Implement configure() for input/output/pullup/pulldown
- [ ] 3.10 Validate against GpioPin concept

## 4. ESP32 UART Implementation

- [ ] 4.1 Create `src/hal/esp32/uart.hpp`
- [ ] 4.2 Create `src/hal/esp32/uart.cpp`
- [ ] 4.3 Implement UART initialization (UART0 for console)
- [ ] 4.4 Configure UART clock divider
- [ ] 4.5 Implement read_byte() using UART_FIFO
- [ ] 4.6 Implement write_byte() using UART_FIFO
- [ ] 4.7 Implement available() checking UART_STATUS
- [ ] 4.8 Implement configure() for baud rate, format
- [ ] 4.9 Validate against UartDevice concept

## 5. Board Definition

- [ ] 5.1 Create `cmake/boards/esp32_devkit.cmake`
- [ ] 5.2 Set ALLOY_MCU to "ESP32"
- [ ] 5.3 Set clock frequency (240MHz max)
- [ ] 5.4 Define Flash size (4MB typical)
- [ ] 5.5 Define RAM size (520KB SRAM)
- [ ] 5.6 Map LED pin (GPIO2 on DevKitC)
- [ ] 5.7 Map UART pins (GPIO1/GPIO3 for UART0)

## 6. Startup and FreeRTOS Integration

- [ ] 6.1 Decide: bare-metal or use FreeRTOS
- [ ] 6.2 If FreeRTOS: create FreeRTOS task wrapper for main()
- [ ] 6.3 If bare-metal: create minimal startup (challenging on ESP32)
- [ ] 6.4 Initialize WiFi/Bluetooth (optional, can be disabled)
- [ ] 6.5 Configure partition table for flash layout

## 7. Examples and Testing

- [ ] 7.1 Build blinky for ESP32 (LED on GPIO2)
- [ ] 7.2 Build uart_echo for ESP32
- [ ] 7.3 Test on ESP32-DevKitC hardware
- [ ] 7.4 Document flash procedure (esptool.py)
