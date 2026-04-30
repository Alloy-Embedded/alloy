# Porting a New Platform

## Goal

A new platform should add as little handwritten runtime logic as possible.

Before adding a platform:

- make sure `alloy-codegen` and `alloy-devices` already publish a typed descriptor contract for that family
- verify whether the platform fits an existing runtime backend schema

## Preferred Order

1. Extend `alloy-codegen` and `alloy-devices` first.
2. Reuse an existing runtime schema backend if possible.
3. Only add runtime code for genuinely new hardware semantics.
4. Add compile smoke and board coverage.

## Avoid

- family-specific switches spread across the runtime
- handwritten register offsets already available in generated descriptors
- string parsing in the runtime when typed refs exist

## GPIO — Open Schema Pattern

Alloy maps GPIO register layouts to C++ *schema types* that satisfy the
`GpioSchemaImpl` concept (`src/hal/detail/gpio_schema_concept.hpp`).
Each schema is a struct with only `static consteval` methods — zero runtime
overhead.

### Required concept methods

```cpp
// All return rt::FieldRef — a {reg_id, bit_offset, bit_width, valid} tuple.
static consteval auto mode_field       (uintptr_t base, int pin) -> rt::FieldRef;
static consteval auto input_data_field (uintptr_t base, int pin) -> rt::FieldRef;
static consteval auto output_set_field (uintptr_t base, int pin) -> rt::FieldRef;
static consteval auto output_clear_field(uintptr_t base, int pin) -> rt::FieldRef;
static consteval auto pull_field       (uintptr_t base, int pin) -> rt::FieldRef;
static constexpr  const char* kSchemaName;
```

Optional (concept check skipped if absent):
`open_drain_field`, `speed_field`, `af_field`.

Return `kInvalidFieldRef` for any field whose IP block puts that control in a
separate subsystem (e.g. RP2040 pull is in PADS, not SIO).

### Registering a new schema

1. Create `src/hal/detail/gpio/<vendor>_gpio_schema.hpp`.
2. Add `static_assert(GpioSchemaImpl<YourSchema>)` in that file.
3. Map the IR `gpio_schema_id` string to your type in
   `alloy-devices/codegen/alloy_cpp_emit/emitter.py` (`_GPIO_SCHEMA_CPP_TYPE`).
4. The codegen template (`gpio.hpp.j2`) will emit `using schema_type = YourSchema`
   inside the per-pin `GpioSemanticTraits` specialization automatically.
5. `pin_handle::schema_type` resolves to your type; the generic backend path in
   `src/hal/gpio/detail/backend.hpp` dispatches via concept methods.

### Backward-compat bridge

If `gpio_schema_id` is absent from the IR, `pin_handle` falls back to
`GpioSchemaTypeFor<GpioSchema enum>` mapping (ST/Microchip/NXP).
New vendors must always supply `gpio_schema_id`.

---

## IRQ — Architecture Implementation Requirements

Alloy's IRQ HAL is split into three layers.  Each new architecture must
provide at most one platform-specific file.

### Layer 1 — `IrqVectorTraits<PeripheralId>` (codegen)

The codegen emits `IrqVectorTraits` specializations from the IR `irq_table`
block.  No hand-written code needed unless the device IR is missing IRQ data.

Fields:
- `kPresent` — `true` when the peripheral has an IRQ vector.
- `kIrqId` — `make_irq_id(n)` where `n` is the Cortex-M / RISC-V IRQ number.
- `kVectorName` — string for ALLOY_IRQ_HANDLER macro expansion.

### Layer 2 — enable/disable/priority backend

Add a file under `src/hal/irq/detail/<arch>/`:

| Architecture | File               | What to implement |
|--------------|--------------------|-------------------|
| Cortex-M     | `nvic_impl.hpp`    | CMSIS `NVIC_*` wrappers |
| RISC-V PLIC  | `plic_impl.hpp`    | PLIC enable + priority registers |
| Xtensa       | `xthal_impl.hpp`   | `xt_ints_on` / `xt_set_intlevel` |
| AVR          | `avr_impl.hpp`     | sei/cli + mask register |

Selected at compile time via `__ARM_ARCH` / `__riscv` / `__XTENSA__` guards in
`src/hal/irq/irq_handle.hpp`.

### Layer 3 — weak ISR dispatch (`default_handler.cpp`)

`src/hal/irq/default_handler.cpp` provides 128 weak `ALLOY_IRQ_SLOT_n_Handler`
stubs (Cortex-M).  Each reads ICSR.VECTACTIVE and calls
`alloy_default_irq_dispatch(irqn)` which looks up the RAM vector table.

For non-Cortex-M targets add a parallel `#elif defined(__riscv)` block that
reads the MCAUSE CSR bits [5:0] and calls the same dispatch function.

### Linker integration

The startup file must alias each peripheral vector name
(`USART2_IRQHandler` etc.) to the matching `ALLOY_IRQ_SLOT_n_Handler` weak
symbol.  `ALLOY_IRQ_HANDLER(PeripheralId)` then overrides the specific slot
with a strongly-linked function.

---

## Clock Tree — IR Section Requirements

Add a `clock_tree` block to the device IR.  `alloy-ir-validate` enforces its
presence when RCC data is available in the SVD.

Minimum required fields per peripheral entry:

```json
{
  "peripheral_id": "Usart2",
  "bus": "apb1",
  "kernel_clock_mux": {
    "reg": "RCC_CCIPR",
    "bit_offset": 2,
    "bit_width": 2,
    "values": { "pclk": 0, "sysclk": 1, "hsi16": 2, "lse": 3 }
  },
  "enable_bit": { "reg": "RCC_APBENR1", "bit_offset": 17 }
}
```

`ClockSemanticTraits<PeripheralId>` is emitted from this data by
`alloy-cpp-emit`.  `peripheral_frequency<P>()` reads the live AHB/APB
divider registers at runtime; no compile-time constant needed.

For devices without a clock-tree IR block, `peripheral_frequency<P>()` returns
`core::Err(core::ErrorCode::NotSupported)` and a `static_assert` fires at
the call site directing the porter to add the IR data.

---

## References

- [ARCHITECTURE.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/ARCHITECTURE.md)
- [CODE_GENERATION.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CODE_GENERATION.md)
- [IRQ_HAL.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/IRQ_HAL.md)
- [CLOCK_HAL.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/CLOCK_HAL.md)
