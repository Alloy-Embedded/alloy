## Why

`alloy` already has the right runtime boundary, but it is not yet the best-in-class embedded
runtime because the public HAL surface is still incomplete and uneven.

The main gaps are:

- not every core peripheral class has a finished, production-grade public API
- DMA is not yet a first-class data plane across the main peripherals
- shared-bus configuration for SPI/I2C is not yet a clean public story
- boards and examples still do not prove the full public runtime path everywhere

This is the biggest direct gap versus `modm`, `Embassy`, vendor BSPs, and STM32Cube.

## What Changes

- complete the primary public HAL surface for foundational peripheral classes:
  - UART
  - SPI
  - I2C
  - DMA
  - Timer
  - PWM
  - ADC
  - DAC
  - RTC
  - CAN
  - Watchdog
- define DMA as the common data plane for the peripheral classes that can use it
- add a first-class shared-bus configuration model for SPI and I2C
- require board helpers and canonical examples to use the public HAL path instead of private glue
- remove residual handwritten family-private peripheral glue from the active runtime path

## Outcome

After this change, `alloy` should present one coherent descriptor-driven runtime API for the
peripherals most users touch first, with DMA and shared-bus support treated as normal features
rather than side paths.

## Impact

- Affected specs:
  - `public-hal-api`
  - `board-bringup`
  - `zero-overhead-runtime`
  - `migration-cleanup`
- Affected code:
  - `src/hal/**`
  - `src/device/**`
  - `boards/**`
  - `examples/**`
  - `tests/**`
