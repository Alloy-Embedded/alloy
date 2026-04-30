# Add Peripheral Clock Gate to configure()

## Why

Several HAL `configure()` methods write to peripheral registers without first
enabling the peripheral's clock gate in the RCC/PMC. The result is silent
failure: MMIO writes succeed (the AHB/AXI bus responds), but the peripheral
never operates because its clock is gated off.

**Validated failure:** SAME70 AFEC0 `read_sequence` returns
`AdcConversionTimeout` after a clean `configure()` + `start()` because the
AFEC peripheral clock is never enabled in the PMC — every MMIO write is
accepted but the converter never triggers.

The infrastructure to fix this is **already complete**:

- `alloy-codegen` emits `PeripheralClockBindingTraits<P>` for every peripheral
  in `clock_bindings.hpp`, mapping each `PeripheralId` to its RCC/PMC clock
  gate bit and reset control.
- `hal::detail::runtime::enable_peripheral_runtime_typed<P>()` (in
  `src/hal/detail/runtime_ops.hpp`) uses those traits to write the enable bit
  and release reset in one call. It is `if constexpr`-gated, so it compiles to
  nothing when no binding is present.
- UART, SPI, I2C, RTC, and DMA already call this function in their
  `configure()` / `open()` paths.

The only fix required is to wire the same call into the `configure()` methods
of the remaining peripherals.

## Affected peripherals

| HAL file | Status |
|---|---|
| `src/hal/uart/detail/backend.hpp` | ✓ already done |
| `src/hal/spi/detail/backend.hpp` | ✓ already done |
| `src/hal/i2c/detail/backend.hpp` | ✓ already done |
| `src/hal/rtc.hpp` | ✓ already done |
| `src/hal/dma/detail/backend.hpp` | ✓ already done |
| `src/hal/adc.hpp` | ✗ missing |
| `src/hal/dac.hpp` | ✗ missing |
| `src/hal/timer.hpp` | ✗ missing |
| `src/hal/can.hpp` | ✗ missing |
| `src/hal/pwm.hpp` | ✗ missing |
| `src/hal/gpio/detail/backend.hpp` | ✗ missing (GPIO port clock) |
| `src/hal/watchdog.hpp` | out of scope — WDT clock is always-on |

## What Changes

### Pattern to add

Every missing `configure()` inserts this block as its **first step**, before any
register writes:

```cpp
if constexpr (Peripheral != device::PeripheralId::none) {
    if (const auto clk = hal::detail::runtime::enable_peripheral_runtime_typed<Peripheral>();
        clk.is_err()) {
        return clk;
    }
}
```

`enable_peripheral_runtime_typed<P>()` is idempotent — writing 1 to an already-
enabled RCC/PMC clock gate bit is a no-op on all supported families. Calling
`configure()` more than once is therefore safe.

### ADC (`src/hal/adc.hpp`)

`handle<Peripheral>::configure()` calls `enable()` then optionally `start()`.
Insert the clock-gate call at the top of `configure()`, before `enable()`.

### DAC (`src/hal/dac.hpp`)

`handle<Peripheral, Channel>::configure()` currently calls `enable()` on the
channel. Insert clock-gate call at the top of `configure()`.

### Timer (`src/hal/timer.hpp`)

`handle<Peripheral>::configure()` calls `set_period()` then `start()`.
Insert clock-gate call at the top of `configure()`.

### CAN (`src/hal/can.hpp`)

`handle<Peripheral>::configure()` programs bit-timing and mode registers.
Insert clock-gate call at the top of `configure()`.

### PWM (`src/hal/pwm.hpp`)

`handle<Peripheral>::configure()` delegates to `channel.configure()`.
Insert clock-gate call at the top of `configure()` (peripheral level only).

### GPIO (`src/hal/gpio/detail/backend.hpp`)

`configure_gpio<PinHandle>()` dispatches to `configure_st_gpio` /
`configure_microchip_pio`. Insert clock-gate call at the top of
`configure_gpio`, keyed on `PinHandle::port_peripheral_id` (the GPIO
port peripheral, e.g. `PeripheralId::GPIOA`). Skip when
`port_peripheral_id == PeripheralId::none`.

> Note: for pins already routed through a peripheral signal (UART, SPI, etc.),
> the peripheral's own `configure()` already enables the peripheral clock.
> The GPIO port clock is separate and still needs enabling on STM32/SAME70.

## Impact

- Zero overhead when the binding is absent (already-on peripherals such as
  always-on system peripherals compile the call away via `if constexpr`).
- Fixes `read_sequence` / `ready()` timeouts on SAME70 AFEC.
- Fixes silent failures on any platform where peripheral clocks are gated at
  reset (STM32, SAME70, NXP iMXRT).
- RP2040 / ESP32 peripherals: `PeripheralClockBindingTraits` carries `kPresent
  = false` for unconfigured bindings → call compiles to `Ok()`, no behaviour
  change.
- No ABI impact; all changes are in header-only inline paths.
