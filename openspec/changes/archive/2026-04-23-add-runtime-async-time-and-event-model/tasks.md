## 1. OpenSpec Baseline

- [x] 1.1 Add the `runtime-async-model` capability
- [x] 1.2 Add public HAL, startup, and zero-overhead deltas

## 2. Time Service

- [x] 2.1 Introduce monotonic `Instant` and `Duration` types or equivalents
- [x] 2.2 Add timer/deadline/timeout primitives on top of the generated timing contract
- [x] 2.3 Prove the time model on foundational families

## 3. Event And Completion Model

- [x] 3.1 Add typed event/completion primitives for IRQ and DMA-backed operations
- [x] 3.2 Connect generated interrupt contracts and startup stubs to the runtime event model
- [x] 3.3 Add representative driver integrations for UART/SPI/I2C/DMA or timer-backed paths

## 4. Optional Async Layer

- [x] 4.1 Add executor-agnostic async adapters for the public HAL
- [x] 4.2 Keep blocking APIs as the primary default path
- [x] 4.3 Prove that blocking-only builds do not pay for async support

## 5. Low Power And Wakeup

- [x] 5.1 Define runtime low-power entry and wakeup coordination hooks
- [x] 5.2 Connect timer/event validity to low-power modes where supported
- [x] 5.3 Add representative board smoke coverage for wakeup-capable paths

## 6. Docs And Examples

- [x] 6.1 Add canonical examples for timeouts, periodic timers, and event-driven completion
- [x] 6.2 Document the blocking/event/async layering clearly

## 7. Validation

- [x] 7.1 Add compile and behavioral validation for the new time/event model
- [x] 7.2 Re-run zero-overhead gates for blocking-only hot paths
- [x] 7.3 Validate the change with `openspec validate add-runtime-async-time-and-event-model --strict`
