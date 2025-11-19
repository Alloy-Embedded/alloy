## Why

Support for STM32F103 (BluePill board) - an extremely popular and cheap ARM Cortex-M3 development board. Different from STM32F4 (different peripherals, clock tree, no FPU).

## What Changes

- Create STM32F103 HAL implementation in `src/hal/stm32f1/`
- Implement GPIO for STM32F1 (different from F4: uses CRL/CRH instead of MODER)
- Implement UART for STM32F1 (USART1/2/3)
- Add STM32F1 CMSIS headers
- Create board definition for BluePill (STM32F103C8T6)
- Integrate with ST's STM32F1 register definitions

## Impact

- Affected specs: hal-stm32f103 (new capability)
- Affected code: src/hal/stm32f1/, cmake/boards/bluepill.cmake
- Validates STM32 family support across different generations
