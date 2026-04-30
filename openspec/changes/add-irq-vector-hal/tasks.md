# Tasks: IRQ Vector HAL

Host-testable: phases 1–3. Phase 4 requires hardware.

## 1. Codegen — IrqVectorTraits

- [ ] 1.1 Define `IrqVectorTraits<PeripheralId>` in codegen IR schema.
      Fields: `kIrqId` (IrqId strong typedef), `kPresent` (bool).
      Source: IR `interrupts` block (already present in SVD-derived IR for Cortex-M).
- [ ] 1.2 Extend `alloy-cpp-emit` irq template to emit `IrqVectorTraits`
      specializations for all peripherals with IRQ data in the IR.
- [ ] 1.3 Regen STM32G0 + STM32F4; verify `IrqVectorTraits<PeripheralId::Usart2>::kIrqId`
      matches CMSIS IRQn value.
- [ ] 1.4 Add `IrqId` strong typedef (`using IrqId = detail::StrongInt<uint16_t>`) to
      `src/hal/irq/irq_id.hpp`.

## 2. HAL — enable/disable/set_priority

- [ ] 2.1 Create `src/hal/irq/irq_handle.hpp`:
      `enable<P>(priority)`, `disable<P>()`, `set_priority<P>(priority)`.
- [ ] 2.2 Implement `src/hal/irq/detail/cortex_m/nvic_impl.hpp`:
      wrap CMSIS `NVIC_SetPriority`, `NVIC_EnableIRQ`, `NVIC_DisableIRQ`.
- [ ] 2.3 Implement `src/hal/irq/detail/riscv/plic_impl.hpp`:
      wrap PLIC enable/priority registers. Placeholder OK for initial commit.
- [ ] 2.4 Implement `src/hal/irq/detail/xtensa/xthal_impl.hpp`: placeholder.
- [ ] 2.5 Implement `src/hal/irq/detail/avr/avr_impl.hpp`: sei/cli + mask register.
- [ ] 2.6 Add compile test `tests/compile_tests/test_irq_hal.cpp`:
      call `irq::enable<PeripheralId::Usart2>(3)`,
      `irq::disable<PeripheralId::Usart2>()` on host (stub NVIC).

## 3. HAL — handler registration

- [ ] 3.1 Create `src/hal/irq/irq_table.hpp`:
      RAM vector table (128 entries default), `set_handler<P>(IrqHandler)`.
- [ ] 3.2 Create `src/hal/irq/default_handler.cpp`:
      weak default ISR that dispatches to RAM table entry or spins in debug loop.
- [ ] 3.3 Define `ALLOY_IRQ_HANDLER(PeripheralId)` macro generating the
      `extern "C" void <VectorName>_IRQHandler()` override.
- [ ] 3.4 Add compile test: `ALLOY_IRQ_HANDLER(PeripheralId::Usart2) { }` compiles
      and links without duplicate-symbol error.

## 4. Hardware validation

- [ ] 4.1 nucleo_g071rb: register UART2 IRQ handler via `set_handler<PeripheralId::Usart2>`.
      Enable IRQ with `irq::enable<PeripheralId::Usart2>(4)`.
      Verify ISR fires on received byte (loopback test).
- [ ] 4.2 nucleo_g071rb: verify `irq::disable<PeripheralId::Usart2>()` stops ISR delivery.
- [ ] 4.3 nucleo_g071rb: EXTI GPIO IRQ via `irq::enable<PeripheralId::Exti13>(5)`;
      button press triggers callback.

## 5. Documentation

- [ ] 5.1 `docs/IRQ_HAL.md`: enable/disable API, handler registration, ALLOY_IRQ_HANDLER
      macro, porting guide for new architectures.
- [ ] 5.2 `docs/PORTING_NEW_PLATFORM.md`: add IRQ abstraction impl requirements.
- [ ] 5.3 Update `docs/ASYNC_HAL.md` (future spec): reference irq HAL as dependency.
