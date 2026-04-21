## 1. Public HAL Surface

- [ ] 1.1 Finish the primary public API shape for UART, SPI, I2C, DMA, timer, PWM, ADC, DAC, RTC, CAN, and watchdog
- [ ] 1.2 Ensure each peripheral class uses one main configuration object or equivalent primary entry path
- [ ] 1.3 Remove or isolate overlapping helper APIs that recreate multiple public tiers

## 2. DMA As Data Plane

- [ ] 2.1 Make DMA configuration first-class in UART, SPI, I2C, ADC, DAC, and PWM where semantics support it
- [ ] 2.2 Route DMA through typed runtime descriptors only
- [ ] 2.3 Add representative DMA-backed examples or smoke paths for supported foundational peripherals

## 3. Shared Bus

- [ ] 3.1 Add explicit shared-bus support for SPI
- [ ] 3.2 Add explicit shared-bus support for I2C
- [ ] 3.3 Support per-device bus reconfiguration through the public runtime path where hardware allows it

## 4. Board And Example Migration

- [ ] 4.1 Migrate foundational board helpers to the public HAL path for the peripherals covered here
- [ ] 4.2 Make canonical examples use the same APIs
- [ ] 4.3 Add missing examples for any foundational peripheral class that still lacks a public demonstration

## 5. Cleanup

- [ ] 5.1 Delete or isolate residual handwritten family-private peripheral glue in the active path
- [ ] 5.2 Update docs to present the completed public HAL surface as the default story

## 6. Validation

- [ ] 6.1 Extend compile smoke coverage across foundational families
- [ ] 6.2 Add behavioral validation for representative DMA and shared-bus paths
- [ ] 6.3 Re-run zero-overhead gates for affected hot paths
- [ ] 6.4 Validate the change with `openspec validate complete-runtime-hal-and-data-plane --strict`
