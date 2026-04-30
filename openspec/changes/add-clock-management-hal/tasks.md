# Tasks: Clock Management HAL

Host-testable phases: 1–4. Phase 5 requires hardware validation.

## 1. Codegen — ClockSemanticTraits

- [ ] 1.1 Define `ClockSemanticTraits<PeripheralId>` struct in codegen IR schema.
      Fields: `kKernelClockMuxField`, `kEnableReg`, `kBusDividerChain`
      (ordered list of RCC divider field refs from SYSCLK down to peripheral bus).
- [ ] 1.2 Extend `alloy-cpp-emit` clock template to emit `ClockSemanticTraits`
      specializations for all peripherals with clock-tree data in the IR.
- [ ] 1.3 Regen STM32G0 + STM32F4 families; verify `ClockSemanticTraits<PeripheralId::Usart2>`
      contains valid `kKernelClockMuxField`.
- [ ] 1.4 Add `clock_tree` to `alloy-ir-validate` required sections for targets
      where RCC data is present in SVD.

## 2. HAL — peripheral_frequency

- [x] 2.1 Create `src/hal/clock/peripheral_frequency.hpp`.
      Reads active AHB/APB divider registers + SYSCLK at runtime; computes bus Hz.
      Returns `core::Result<uint32_t, core::ErrorCode>`.
- [x] 2.2 Specialize for targets with no clock-tree traits: return
      `core::Err(core::ErrorCode::NotSupported)` with static_assert message
      directing user to add clock-tree IR data.
- [ ] 2.3 Update `uart_handle.hpp`: replace `set_baud(baud, pclk_hz)` with
      `set_baud(baud)` that calls `peripheral_frequency<Peripheral::id>()`.
      Keep deprecated two-argument overload.
- [ ] 2.4 Update `spi_handle.hpp`: same pattern for prescaler calculation.
- [x] 2.5 Add compile test `tests/compile_tests/test_clock_hal.cpp`:
      call `clock::peripheral_frequency<PeripheralId::none>()`;
      call `set_kernel_clock(KernelClockSource::pclk)`.

## 3. HAL — kernel clock mux

- [x] 3.1 Create `src/hal/clock/kernel_clock.hpp`.
      `set_kernel_clock<P>(KernelClockSource)` writes `kKernelClockMuxField`.
      Guarded by `if constexpr (ClockSemanticTraits<P::id>::kKernelClockMuxField.valid)`.
- [x] 3.2 Define `KernelClockSource` enum in `src/hal/clock/kernel_clock_source.hpp`.
      Vendor-neutral values: `pclk`, `hsi16`, `lse`, `sysclk`, `pll2_q`, `hse`.
      Vendor schemas map enum → 2-bit or 3-bit field value.
- [ ] 3.3 Add compile test: call `clock::set_kernel_clock<PeripheralId::Usart2>(KernelClockSource::hsi16)`.
      (Blocked on ClockSemanticTraits codegen — task 1.2)

## 4. HAL — profile switching

- [x] 4.1 Create `src/hal/clock/clock_profile.hpp` with `ClockProfile` struct.
- [x] 4.2 Implement `switch_profile(const ClockProfile&)` for STM32G0.
      Sequence: increase wait states → ramp PLL → switch SYSCLK src → reduce wait states.
- [x] 4.3 Emit `profiles::default_pll_64mhz`, `profiles::low_power_hsi_4mhz` for
      STM32G071 from IR clock profile data in board_config.hpp.
      (API: switch_to_default_profile() / switch_to_safe_profile() delegate to device layer)
- [ ] 4.4 Add compile test: call `clock::switch_profile(profiles::default_pll_64mhz)`.
      (Blocked on IR ClockProfile → named profile alias codegen)

## 5. Hardware validation

- [ ] 5.1 nucleo_g071rb: verify UART at 115200 baud with auto-queried pclk.
      Compare with hardcoded-constant baseline — same behavior.
- [ ] 5.2 nucleo_g071rb: call `switch_profile(low_power_hsi_4mhz)`; verify UART
      baud recalculates and still works at 115200 (lower speed).
- [ ] 5.3 nucleo_f401re: `set_kernel_clock<Usart2>(KernelClockSource::hsi16)`;
      verify UART still works at 115200 with HSI kernel clock.

## 6. Documentation

- [ ] 6.1 `docs/CLOCK_HAL.md`: how peripheral_frequency works, when to call
      switch_profile, vendor extension points for KernelClockSource.
- [ ] 6.2 `docs/PORTING_NEW_PLATFORM.md`: add clock-tree IR section requirements.
- [ ] 6.3 Deprecation notice in `uart_handle.hpp` / `spi_handle.hpp` pointing to
      new zero-argument `set_baud`.
