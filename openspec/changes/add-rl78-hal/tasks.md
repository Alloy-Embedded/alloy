## 1. RL78 Toolchain Setup

- [ ] 1.1 Research RL78 toolchain options (CC-RL vs GNURL78)
- [ ] 1.2 Create `cmake/toolchains/rl78-gcc.cmake` for GNURL78
- [ ] 1.3 Set RL78-specific compiler flags
- [ ] 1.4 Configure linker script template for RL78

## 2. RL78 Register Definitions

- [ ] 2.1 Obtain RL78 iodefine.h (Renesas headers)
- [ ] 2.2 Create wrapper in `src/platform/rl78/` for register access
- [ ] 2.3 Define port registers (P0-P15, PM0-PM15)
- [ ] 2.4 Define UART registers (SAU0, SAU1)

## 3. RL78 GPIO Implementation

- [ ] 3.1 Create `src/hal/rl78/gpio.hpp`
- [ ] 3.2 Create `src/hal/rl78/gpio.cpp`
- [ ] 3.3 Implement port-based GPIO (RL78 uses ports, not individual pin control)
- [ ] 3.4 Map generic pin numbers to port/bit pairs
- [ ] 3.5 Implement set_high(), set_low(), toggle(), read()
- [ ] 3.6 Implement configure() for input/output/pullup modes
- [ ] 3.7 Validate against GpioPin concept

## 4. RL78 UART Implementation

- [ ] 4.1 Create `src/hal/rl78/uart.hpp`
- [ ] 4.2 Create `src/hal/rl78/uart.cpp`
- [ ] 4.3 Implement SAU (Serial Array Unit) initialization
- [ ] 4.4 Implement read_byte() using SAU receive
- [ ] 4.5 Implement write_byte() using SAU transmit
- [ ] 4.6 Implement available() checking SAU status
- [ ] 4.7 Implement configure() for baud rate, parity, stop bits
- [ ] 4.8 Validate against UartDevice concept

## 5. Board Definition

- [ ] 5.1 Create `cmake/boards/cf_rl78.cmake`
- [ ] 5.2 Define MCU type (e.g., R5F100LE)
- [ ] 5.3 Set clock frequency (typically 32MHz max)
- [ ] 5.4 Define memory layout (Flash/RAM sizes)
- [ ] 5.5 Map LED pin for blinky example
- [ ] 5.6 Map UART pins for console

## 6. Startup Code

- [ ] 6.1 Create `src/platform/rl78/startup.c` (RL78 uses C, not C++)
- [ ] 6.2 Implement reset handler
- [ ] 6.3 Implement interrupt vector table for RL78
- [ ] 6.4 Initialize .data and .bss sections
- [ ] 6.5 Call main() after initialization

## 7. Examples and Testing

- [ ] 7.1 Build blinky for RL78 target
- [ ] 7.2 Build uart_echo for RL78 target
- [ ] 7.3 Test on actual CF_RL78 hardware
- [ ] 7.4 Document flash procedure (E2 Lite debugger)
