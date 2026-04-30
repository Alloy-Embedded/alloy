# Tasks: IRQ Vector HAL

Host-testable: phases 1–3. Phase 4 requires hardware.

## 1. Codegen — IrqVectorTraits

- [x] 1.1 Define `IrqVectorTraits<PeripheralId>` in codegen IR schema.
      Fields: `kIrqId` (IrqId strong typedef), `kPresent` (bool).
      Source: IR `interrupts` block (already present in SVD-derived IR for Cortex-M).
- [ ] 1.2 Extend `alloy-cpp-emit` irq template to emit `IrqVectorTraits`
      specializations for all peripherals with IRQ data in the IR.
      (Codegen template: templates/irq_vector_traits.hpp.j2)
- [ ] 1.3 Regen STM32G0 + STM32F4; verify `IrqVectorTraits<PeripheralId::Usart2>::kIrqId`
      matches CMSIS IRQn value.
- [x] 1.4 Add `IrqId` strong typedef (`struct IrqId { std::uint16_t value; }`) to
      `src/hal/irq/irq_id.hpp`. Add make_irq_id() factory.

## 2. HAL — enable/disable/set_priority

- [x] 2.1 Create `src/hal/irq/irq_handle.hpp`:
      `enable<P>(priority)`, `disable<P>()`, `set_priority<P>(priority)`.
- [x] 2.2 Implement `src/hal/irq/detail/cortex_m/nvic_impl.hpp`:
      wrap CMSIS `NVIC_SetPriority`, `NVIC_EnableIRQ`, `NVIC_DisableIRQ`.
- [x] 2.3 Implement `src/hal/irq/detail/riscv/plic_impl.hpp`:
      wrap PLIC enable/priority registers. Placeholder OK for initial commit.
- [x] 2.4 Implement `src/hal/irq/detail/xtensa/xthal_impl.hpp`: placeholder.
- [x] 2.5 Implement `src/hal/irq/detail/avr/avr_impl.hpp`: sei/cli + mask register.
- [x] 2.6 Add compile test `tests/compile_tests/test_irq_hal.cpp`:
      call `irq::enable<PeripheralId::none>(3)`,
      `irq::disable<PeripheralId::none>()` on host (stub NVIC).

## 3. HAL — handler registration

- [x] 3.1 Create `src/hal/irq/irq_table.hpp`:
      RAM vector table (128 entries default), `set_handler<P>(IrqHandler)`.
- [ ] 3.2 Create `src/hal/irq/default_handler.cpp`:
      weak default ISR that dispatches to RAM table entry or spins in debug loop.
      (Defined inline in irq_table.hpp for now; .cpp needed for linker weak override)
- [x] 3.3 Define `ALLOY_IRQ_HANDLER(PeripheralId)` macro generating the
      `extern "C" void <VectorName>_IRQHandler()` override.
      (In irq_table.hpp; vector name from IrqVectorTraits::kVectorName)
- [x] 3.4 Add compile test: `test_irq_hal.cpp` verifies IRQ HAL types and API compile.

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
