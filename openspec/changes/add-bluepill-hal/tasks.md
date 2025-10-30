## 1. STM32F1 Toolchain Setup

- [ ] 1.1 Verify arm-none-eabi-gcc works for Cortex-M3
- [ ] 1.2 Create `cmake/toolchains/arm-cortex-m3.cmake`
- [ ] 1.3 Set -mcpu=cortex-m3 -mthumb flags
- [ ] 1.4 Configure linker script for STM32F103C8T6

## 2. CMSIS Headers for STM32F1

- [ ] 2.1 Obtain STM32F1 CMSIS headers from ST
- [ ] 2.2 Add to `external/cmsis/Device/ST/STM32F1xx/`
- [ ] 2.3 Include stm32f1xx.h and stm32f103xb.h
- [ ] 2.4 Configure system_stm32f1xx.c for clock setup

## 3. STM32F1 GPIO Implementation

- [ ] 3.1 Create `src/hal/stm32f1/gpio.hpp`
- [ ] 3.2 Create `src/hal/stm32f1/gpio.cpp`
- [ ] 3.3 Implement GPIO using CRL/CRH registers (F1 specific)
- [ ] 3.4 Map pin numbers to GPIOx ports and bits
- [ ] 3.5 Implement set_high() using BSRR register
- [ ] 3.6 Implement set_low() using BRR register
- [ ] 3.7 Implement toggle() using ODR XOR
- [ ] 3.8 Implement read() using IDR register
- [ ] 3.9 Implement configure() for input/output/alternate modes
- [ ] 3.10 Validate against GpioPin concept

## 4. STM32F1 UART Implementation

- [ ] 4.1 Create `src/hal/stm32f1/uart.hpp`
- [ ] 4.2 Create `src/hal/stm32f1/uart.cpp`
- [ ] 4.3 Implement USART initialization (USART1 on BluePill)
- [ ] 4.4 Configure RCC for USART clock enable
- [ ] 4.5 Implement read_byte() using DR register
- [ ] 4.6 Implement write_byte() using DR register
- [ ] 4.7 Implement available() checking RXNE flag
- [ ] 4.8 Implement configure() for baud rate calculation
- [ ] 4.9 Validate against UartDevice concept

## 5. Board Definition

- [ ] 5.1 Create `cmake/boards/bluepill.cmake`
- [ ] 5.2 Set ALLOY_MCU to "STM32F103C8T6"
- [ ] 5.3 Set clock frequency (72MHz max with HSE)
- [ ] 5.4 Define Flash size (64KB for C8, 128KB for CB)
- [ ] 5.5 Define RAM size (20KB)
- [ ] 5.6 Map LED pin (PC13 on BluePill)
- [ ] 5.7 Map UART pins (PA9/PA10 for USART1)

## 6. Startup Code

- [ ] 6.1 Reuse ARM Cortex-M startup template
- [ ] 6.2 Customize vector table for STM32F103
- [ ] 6.3 Implement SystemInit() for clock configuration
- [ ] 6.4 Configure PLL for 72MHz from 8MHz HSE

## 7. Examples and Testing

- [ ] 7.1 Build blinky for BluePill (LED on PC13)
- [ ] 7.2 Build uart_echo for BluePill
- [ ] 7.3 Test on actual BluePill hardware
- [ ] 7.4 Document flash procedure (ST-Link, USB bootloader)
