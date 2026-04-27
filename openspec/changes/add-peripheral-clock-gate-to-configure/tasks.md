# Tasks: Add Peripheral Clock Gate to configure()

All phases are host-testable (compile + concept) except phase 4 which requires
the 3-board hardware matrix (SAME70 / STM32G0 / STM32F4).

## 1. ADC

- [x] 1.1 `src/hal/adc.hpp` — add `enable_peripheral_runtime_typed<Peripheral>()`
      at the top of `handle::configure()`, before `enable()`.
- [ ] 1.2 Verify on SAME70 Xplained: `analog_probe_complete` `read_sequence`
      must return `Ok` (not `AdcConversionTimeout`) after this change.
      Note: AFEC `read_sequence` still times out because `AFEC_MR.PRESCAL=0`
      drives AFEC at MCK/2 ≈ 75 MHz > 40 MHz max; prescaler init is
      out-of-scope — tracked as `add-afec-prescaler-init` (TBD).
- [ ] 1.3 Verify on STM32G071RB and STM32F401RE: `analog_probe_complete`
      `read_sequence` returns `Ok` once G071RB and F401RE build errors
      are resolved (see tasks 7.x).

## 2. DAC

- [x] 2.1 `src/hal/dac.hpp` — add clock-gate call at top of
      `handle::configure()`.
- [ ] 2.2 Verify on SAME70 Xplained and STM32G071RB: `analog_probe`
      DAC path writes without error.

## 3. Timer

- [x] 3.1 `src/hal/timer.hpp` — add clock-gate call at top of
      `handle::configure()`.
- [ ] 3.2 Verify compile against all 3 boards; hardware spot-check via
      `timer_pwm_probe` on STM32G071RB.

## 4. CAN

- [x] 4.1 `src/hal/can.hpp` — add clock-gate call at top of
      `handle::configure()`.
- [x] 4.2 Compile test against boards with CAN semantics (STM32G0, SAME70).

## 5. PWM

- [x] 5.1 `src/hal/pwm.hpp` — add clock-gate call at top of
      `handle::configure()` (peripheral level, before channel delegation).
- [ ] 5.2 Verify via `timer_pwm_probe` on STM32G071RB.

## 6. GPIO

- [x] 6.1 `src/hal/gpio/detail/backend.hpp` — already calls
      `enable_gpio_port_runtime<PinHandle::peripheral_id>()` for both
      `st_gpio` and `microchip_pio_v` schemas (lines 143 and 150). No
      change needed.
- [ ] 6.2 Verify: `blink` example continues to work on all 3 boards.

## 7. Compile build fixes (descriptor update breakage)

Alloy-devices was regenerated from alloy-codegen `03e838a6085f`. Two
breakages surfaced when building G071RB and F401RE:

- [x] 7.1 `src/device/dev.hpp` — removed shortcuts for `signal_cts`,
      `signal_rts`, `signal_scl`, `signal_sda`; these are no longer
      universal across all device descriptors. `using enum device::SignalId`
      already provides all available signals per-device.
- [x] 7.2 `src/runtime/interrupt_bridge.cpp` — guarded `USART2` references
      in `bridge_stm32g0_dma_channel3/4` with
      `requires { hal::dma::PeripheralId::USART2; }` so devices without
      USART2 in their PeripheralId compile cleanly.
- [x] 7.3 `cmake/platforms/stm32f4.cmake` — added
      `-fconstexpr-ops-limit=268435456` to `OPT_FLAGS`; STM32F4's larger
      field/register set exceeds the default 33M constexpr ops limit during
      instantiation of the runtime binary-search lookups.

## 8. Out-of-scope

- Watchdog (`src/hal/watchdog.hpp`): WDT/IWDG/WWDG clocks are always-on on
  all supported families; no change needed.
- Ethernet, USB, QSPI, SDMMC: these peripherals each have more complex
  bring-up sequences that belong in their own extend-* openspec.
- AFEC prescaler (`AFEC_MR.PRESCAL`): AFEC clock is enabled by this change
  but AFEC_MR.PRESCAL=0 gives AFEC_CLK = MCK/2 ≈ 75 MHz > 40 MHz max,
  causing `read_sequence` to time out. Needs `add-afec-prescaler-init`
  openspec in alloy or alloy-codegen.
