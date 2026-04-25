## 1. Codegen — Driver Semantics Field Emission

- [x] 1.1 Map every TWIHS CR/SR/MMR/IADR/THR/RHR bit in the source YAML to
      an emitted `RuntimeFieldRef`; zero `kInvalidFieldRef` entries in
      `driver_semantics/i2c.hpp` for fields the HAL references.
- [x] 1.2 Do the same pass for SPI, USART, PMC, and GMAC semantic traits
      headers. (Verified: SAME70-specific fields for all four peripherals
      are valid in the emitted headers; STM32-only fields are correctly absent.)
- [x] 1.3 Re-emit `alloy-devices/microchip/same70/generated/**` and confirm
      field counts against the diff produced by the generator.

## 2. Codegen — Pin-Route Helper

- [x] 2.1 Emit `template <PinId, PeripheralId, SignalId> auto route() -> void`
      that writes the selector already captured in `routes.hpp` through the
      `schema_alloy_pinmux_sam_pio_v1` backend (SAM E70) and the equivalent
      STM32 / NXP backends.
- [x] 2.2 Provide a compile-time diagnostic when the requested
      `(Pin, Peripheral, Signal)` triple has no route entry.

## 3. Codegen — Clock Enable / Disable Helpers

- [x] 3.1 Emit `alloy::clock::enable(PeripheralId)` +
      `alloy::clock::disable(PeripheralId)` that resolve to the correct
      `ClockGateId` + PCER register write.
- [x] 3.2 Cover both PCER0 and PCER1 peripheral id ranges on SAM E70.

## 4. Codegen — Peripheral Base Accessor

- [x] 4.1 Emit `template <PeripheralId Id> constexpr auto base() -> std::uintptr_t`
      returning `PeripheralInstanceTraits<Id>::kBaseAddress`.
- [x] 4.2 Ban raw base-address literals in `src/hal/**` (grep gate in CI).

## 5. Codegen — GPIO-Only Pin Binding

- [ ] 5.1 Allow `alloy::hal::gpio::configure(PinId, Direction, Level)` for
      pins that are not bound to a peripheral signal (reset lines, LEDs,
      strap probes).

## 6. Alloy — TWIHS Backend Refactor

- [x] 6.1 Replace `kTwihsCrStart` / `kTwihsCrStop` / `kTwihsSr*` raw masks in
      `src/hal/i2c/detail/backend.hpp` with the emitted `FieldRef`s from
      task 1.1.
- [ ] 6.2 Re-run `driver_at24mac402_probe` on silicon; confirm PASS.

## 7. Alloy — Probe Refactors

- [x] 7.1 `examples/driver_at24mac402_probe/main.cpp`: drop PMC/PIO MMIO;
      clock enable + pinmux route handled by `configure_i2c()` backend via
      `apply_route_operations` + `enable_peripheral_runtime_typed`.
- [x] 7.2 `examples/driver_is42s16100f_probe/main.cpp`: PMC calls go through
      `alloy::clock::enable<>`; PIO pinmux via raw PeriphA select (no
      SDRAMC alloy driver yet, raw JEDEC init stays inline).
- [x] 7.3 `examples/driver_ksz8081_probe/main.cpp`: `kGmacBase` replaced with
      `alloy::device::base<PeripheralId::GMAC>()`; clock/pinmux via
      `alloy::clock::enable<>` + `alloy::pinmux::route<>`.

## 8. Validation

- [x] 8.1 Add `tests/compile/contract_smoke.cmake` entry that fails if any
      HAL-referenced semantic field resolves to `kInvalidFieldRef`.
- [ ] 8.2 Re-run all three probes on SAME70 Xplained Ultra; confirm PASS.
- [ ] 8.3 `openspec validate close-codegen-semantics-gaps --strict`.
