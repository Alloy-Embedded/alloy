# Tasks: Open GPIO Schema Traits

Phases 1–3 are host-only. Phase 4 requires hardware spot-checks.

## 1. Concept and existing schema migration

- [x] 1.1 Define `GpioSchemaImpl` concept in `src/hal/detail/gpio_schema_concept.hpp`.
      Required methods: `mode_field`, `output_set_field`, `output_clear_field`,
      `input_data_field`, `pull_field`. Optional: `speed_field`, `af_field`, `open_drain_field`.
- [x] 1.2 Implement `StGpioSchema` in `src/hal/detail/gpio/st_gpio_schema.hpp`.
      Port all consteval offset arithmetic from current `pin_handle` ST branch.
      Add `static_assert(GpioSchemaImpl<StGpioSchema>)`.
- [x] 1.3 Implement `MicrochipPioSchema` in `src/hal/detail/gpio/microchip_pio_schema.hpp`.
      Port Microchip PIO_v branch. Add concept assertion.
- [x] 1.4 Implement `NxpImxrtGpioSchema` in `src/hal/detail/gpio/nxp_imxrt_gpio_schema.hpp`.
      Port NXP iMX.RT branch. Add concept assertion.
- [x] 1.5 Add `UnknownGpioSchema` stub that satisfies the concept and returns
      `kInvalidFieldRef` for all fields (graceful degradation for unsupported devices).

## 2. Codegen extension

- [x] 2.1 Extend `alloy-cpp-emit` GPIO template to emit `using schema_type = <Schema>` in
      `GpioSemanticTraits`. Map IR `gpio_schema_id` → C++ schema type name.
      (templates/driver_semantics/gpio.hpp.j2 + _build_gpio_pin_data helper in emitter.py)
- [ ] 2.2 Regen all existing devices; verify `schema_type` appears in generated traits.
      (Deferred — requires per-device IR gpio_schema_id bootstrap patches)
- [x] 2.3 Add IR field `gpio_schema_id` to hal-contracts/gpio.json as optional_scalar.
      (version bumped to 1.1.0)

## 3. HAL migration

- [x] 3.1 Refactor `src/hal/gpio/gpio.hpp` (pin_handle): add schema_type via
      ResolveGpioSchemaType<semantic_traits, schema>; backend.hpp dispatch changed
      from GpioSchema enum to std::is_same_v<Schema, StGpioSchema> etc.
      Generic concept-method configure/write/read path added for new vendors.
- [x] 3.2 Backward-compat fallback: GpioSchemaTypeFor<S> maps enum → type when
      semantic_traits::schema_type absent. schema_type_from_codegen flag tracks source.
- [x] 3.3 Update compile tests (`test_gpio_api.cpp`): schema_type present and is correct
      type for STM32 (StGpioSchema) and SAME70 (MicrochipPioSchema) boards.
- [x] 3.4 Add `tests/compile_tests/test_gpio_schemas.cpp`: instantiate all schema types
      against the concept; verify concept satisfaction.

## 4. New vendor schema (validation + expansion)

- [x] 4.1 Implement `NordicGpioteSchema` in `src/hal/detail/gpio/nordic_gpiote_schema.hpp`
      (nRF52840 PIN_CNF mode/pull, OUTSET/OUTCLR, IN). Concept assertion + field tests.
- [x] 4.2 Implement `Rp2040GpioSchema` for RP2040 SIO GPIO IP.
      (GPIO_OE_SET, GPIO_OUT_SET/CLR, GPIO_IN; no pull — in PADS block)
- [x] 4.3 Implement `EspressifGpioSchema` for ESP32 GPIO matrix IP.
      (GPIO_ENABLE_W1TS/W1TC, GPIO_OUT_W1TS/W1TC, GPIO_IN_REG; no pull — IO_MUX)
- [ ] 4.4 Hardware spot-check: verify GPIO toggle on nucleo_g071rb + same70_xplained
      after HAL migration. (Requires hardware)

## 5. Deprecation cleanup

- [x] 5.1 GpioSchema enum and to_gpio_schema() marked @deprecated in doc-comments.
      Full [[deprecated]] attr deferred to task 5.2 (after all usage sites migrated).
- [ ] 5.2 Follow-up release (semver minor): remove GpioSchema enum + to_gpio_schema().
- [x] 5.3 Document the open schema pattern in `docs/PORTING_NEW_PLATFORM.md`.

## 6. Follow-up specs (out of scope here)

- [ ] 6.1 Open `uart-schema-open-traits` spec — same pattern for UART.
- [ ] 6.2 Open `spi-schema-open-traits` spec — same pattern for SPI.
- [ ] 6.3 Open `i2c-schema-open-traits` spec — same pattern for I2C.
