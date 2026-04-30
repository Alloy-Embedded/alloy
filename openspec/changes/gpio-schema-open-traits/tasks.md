# Tasks: Open GPIO Schema Traits

Phases 1–3 are host-only. Phase 4 requires hardware spot-checks.

## 1. Concept and existing schema migration

- [ ] 1.1 Define `GpioSchemaImpl` concept in `src/hal/detail/gpio_schema_concept.hpp`.
      Required methods: `mode_field`, `output_set_field`, `output_clear_field`,
      `input_data_field`, `pull_field`. Optional: `speed_field`, `af_field`, `open_drain_field`.
- [ ] 1.2 Implement `StGpioSchema` in `src/hal/detail/gpio/st_gpio_schema.hpp`.
      Port all consteval offset arithmetic from current `pin_handle` ST branch.
      Add `static_assert(GpioSchemaImpl<StGpioSchema>)`.
- [ ] 1.3 Implement `MicrochipPioSchema` in `src/hal/detail/gpio/microchip_pio_schema.hpp`.
      Port Microchip PIO_v branch. Add concept assertion.
- [ ] 1.4 Implement `NxpImxrtGpioSchema` in `src/hal/detail/gpio/nxp_imxrt_gpio_schema.hpp`.
      Port NXP iMX.RT branch. Add concept assertion.
- [ ] 1.5 Add `UnknownGpioSchema` stub that satisfies the concept and returns
      `kInvalidFieldRef` for all fields (graceful degradation for unsupported devices).

## 2. Codegen extension

- [ ] 2.1 Extend `alloy-cpp-emit` GPIO template to emit `using schema_type = <Schema>` in
      `GpioSemanticTraits`. Map IR `gpio_schema_id` → C++ schema type name.
- [ ] 2.2 Regen all existing devices; verify `schema_type` appears in generated traits.
- [ ] 2.3 Add IR field `gpio_schema_id` to `alloy-ir-validate` required field list for
      GPIO peripherals.

## 3. HAL migration

- [ ] 3.1 Refactor `src/hal/gpio/pin_handle.hpp`: replace `to_gpio_schema()` dispatch +
      `if constexpr` chains with `using schema = typename semantic_traits::schema_type`.
      Delegate all field access to schema methods.
- [ ] 3.2 Keep backward-compat fallback: if `schema_type` is absent (pre-migration
      devices), fall back to old `kSchemaId` dispatch. Emit a `#warning` to prod
      codegen regen.
- [ ] 3.3 Update compile tests (`test_gpio_api.cpp`) to verify all HAL methods still
      compile. Run host compile check with `StGpioSchema`.
- [ ] 3.4 Add `tests/compile_tests/test_gpio_schemas.cpp`: instantiate all schema types
      against the concept; verify concept satisfaction.

## 4. New vendor schema (validation + expansion)

- [ ] 4.1 Implement `NordicGpioteSchema` in `src/hal/detail/gpio/nordic_gpiote_schema.hpp`
      (required for nRF52840 bring-up). Add concept assertion.
- [ ] 4.2 Implement `RaspberryPiGpioSchema` for RP2040/RP2350 GPIO IP.
      (SIO_BASE + OE_SET/OE_CLR/OUT_SET/OUT_CLR/IN registers per bank)
- [ ] 4.3 Implement `EspressifGpioSchema` for ESP32 GPIO matrix IP.
- [ ] 4.4 Hardware spot-check: verify GPIO toggle on nucleo_g071rb (ST) + same70_xplained
      (Microchip) still works after HAL migration.

## 5. Deprecation cleanup

- [ ] 5.1 After all supported devices regenerated with `schema_type`:
      deprecate `GpioSchema` enum with `[[deprecated]]`.
- [ ] 5.2 In a follow-up release (semver minor): remove `GpioSchema` enum and
      `to_gpio_schema()` function. Remove `if constexpr` enum dispatch from `pin_handle`.
- [ ] 5.3 Document the open schema pattern in `docs/PORTING_NEW_PLATFORM.md` as the
      canonical way to add a new GPIO IP.

## 6. Follow-up specs (out of scope here)

- [ ] 6.1 Open `uart-schema-open-traits` spec — same pattern for UART.
- [ ] 6.2 Open `spi-schema-open-traits` spec — same pattern for SPI.
- [ ] 6.3 Open `i2c-schema-open-traits` spec — same pattern for I2C.
