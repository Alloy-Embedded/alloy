## 1. OpenSpec Baseline

- [ ] 1.1 Add the `runtime-async-model` capability
- [ ] 1.2 Add public HAL, startup, and zero-overhead deltas

## 2. Time Service

- [ ] 2.1 Introduce monotonic `Instant` and `Duration` types or equivalents
- [ ] 2.2 Add timer/deadline/timeout primitives on top of the generated timing contract
- [ ] 2.3 Prove the time model on foundational families

## 3. Event And Completion Model

- [ ] 3.1 Add typed event/completion primitives for IRQ and DMA-backed operations
- [ ] 3.2 Connect generated interrupt contracts and startup stubs to the runtime event model
- [ ] 3.3 Add representative driver integrations for UART/SPI/I2C/DMA or timer-backed paths

## 4. Optional Async Layer

- [ ] 4.1 Add executor-agnostic async adapters for the public HAL
- [ ] 4.2 Keep blocking APIs as the primary default path
- [ ] 4.3 Prove that blocking-only builds do not pay for async support

## 5. Low Power And Wakeup

- [ ] 5.1 Define runtime low-power entry and wakeup coordination hooks
- [ ] 5.2 Connect timer/event validity to low-power modes where supported
- [ ] 5.3 Add representative board smoke coverage for wakeup-capable paths

## 6. Docs And Examples

- [ ] 6.1 Add canonical examples for timeouts, periodic timers, and event-driven completion
- [ ] 6.2 Document the blocking/event/async layering clearly

## 7. Validation

- [ ] 7.1 Add compile and behavioral validation for the new time/event model
- [ ] 7.2 Re-run zero-overhead gates for blocking-only hot paths
- [ ] 7.3 Validate the change with `openspec validate add-runtime-async-time-and-event-model --strict`
