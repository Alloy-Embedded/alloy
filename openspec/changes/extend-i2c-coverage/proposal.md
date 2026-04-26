# Extend I2C Coverage To Match Published Descriptor Surface

## Why

The latest alloy-devices publication ships a rich
`I2cSemanticTraits<P>` covering every CR1 / CR2 / OAR / TIMINGR /
ISR field across ST / Microchip / NXP, plus capability flags and
Tier 2/3/4 metadata (`kIrqNumbers`, `kDmaBindings`,
`kKernelClockSelectorField`, `kKernelMaxClockHz`, kernel-clock
source options, SMBus support, PEC support, clock-stretching
control).

The runtime currently consumes ~25% of that surface: the HAL
exposes `configure / read / write / write_read / scan_bus /
configure_tx_dma / configure_rx_dma`. Speed is implicit (set during
`configure` via `peripheral_clock_hz`), 10-bit addressing isn't
exposed, no clock-stretching control, no SMBus, no PEC, no NACK /
arbitration-loss / bus-error status accessors, no typed interrupt
control.

modm covers I2C end-to-end with master + slave + SMBus + PEC + every
status flag observable. Alloy already publishes the data needed;
this change is plumbing.

Scope: lift every published `I2cSemanticTraits<P>` field and Tier
2/3/4 metadata into typed HAL methods, capability-gated via
`if constexpr`. Wire the IRQ + DMA + kernel-clock levers into the
existing `async::i2c::write / read / write_read` from
`complete-async-hal`. Ship `i2c_probe_complete` example. Document
in `docs/I2C.md`.

## What Changes

### `src/hal/i2c/i2c.hpp` — extended HAL surface (additive only)

- **Speed / timing**
  - `set_clock_speed(std::uint32_t hz)` — re-resolves TIMINGR (or
    CCR + CCFR on legacy STM32) from the current kernel clock.
    Returns `InvalidArgument` if the requested rate cannot be
    realised within ±5 % using the descriptor's TIMINGR encoding.
  - `set_speed_mode(SpeedMode)` — `Standard100kHz`, `Fast400kHz`,
    `FastPlus1MHz`. Clamped against `kSupportsFastPlus`.
  - `set_duty_cycle(DutyCycle)` — `Duty2`, `Duty169` for fast mode;
    gated on `kDutyField.valid`.
- **Addressing**
  - `set_addressing_mode(AddressingMode)` — `Bits7` or `Bits10`.
  - `set_own_address(std::uint16_t addr, AddressingMode mode)` —
    slave own address (single).
  - `set_dual_address(std::uint16_t addr2)` — gated on backends
    that publish a second OAR field.
- **Clock stretching (gated)**
  - `set_clock_stretching(bool enabled)` — gated on the NOSTRETCH
    field.
- **NACK / arbitration / bus-error status**
  - `nack_received() -> bool`, `arbitration_lost() -> bool`,
    `bus_error() -> bool` — typed accessors with `clear_*` mirrors.
- **Kernel clock source (gated)**
  - `set_kernel_clock_source(KernelClockSource)` — same shape as
    UART / SPI variants.
- **SMBus + PEC (gated on `kSupportsSmbus` / `kSupportsPec`)**
  - `enable_smbus(bool)`, `enable_pec(bool)`,
    `last_pec() -> std::uint8_t`, `pec_error() -> bool`,
    `clear_pec_error()`.
- **Interrupts (typed)**
  - `enum class InterruptKind { Tx, Rx, Stop, Tc, AddrMatch, Nack,
    BusError, ArbitrationLoss, Overrun, PecError, Timeout, SmbAlert }`.
  - `enable_interrupt(InterruptKind)` /
    `disable_interrupt(InterruptKind)` — each kind gated.
- **NVIC vector lookup**
  - `irq_numbers() -> std::span<const std::uint32_t>` mirroring
    UART / SPI.
- **Async sibling**
  - `async::i2c::wait_for(InterruptKind)` — extends
    `complete-async-hal`'s I2C path.

### `examples/i2c_probe_complete/`

Targets `nucleo_g071rb` I2C1. Configures Fast Plus 1 MHz with
Duty2, 10-bit addressing, clock-stretching enabled, NACK +
bus-error interrupts, async write_read against an AT24MAC402
EEPROM. Mirrors a modm `I2cMaster` recipe.

### Docs

`docs/I2C.md` — comprehensive guide: model, speed-mode recipe,
10-bit addressing, clock-stretching, SMBus, error handling,
async wiring, modm migration table.

## What Does NOT Change

- Existing I2C API is unchanged. New methods are additive.
- I2C tier in `docs/SUPPORT_MATRIX.md` stays `representative` —
  hardware spot-checks for new levers land per board with the
  existing SAME70 spot-check pattern.
- Descriptor-side fields are NOT changed.

## Out of Scope (Follow-Up Changes)

- **`add-i2c-clock-source-typed-enum` (alloy-codegen).** Mirror of
  the ADC/UART/SPI typed-enum follow-ups.
- **Slave mode end-to-end.** The descriptor publishes the slave-side
  fields (own address, address-match interrupt, address-bit-9 mask);
  shipping a full `I2cSlave<C>` HAL surface lands as a separate
  change once the design questions (callback shape, multi-slave
  arbitration) settle.
- **DMA double-buffer / circular RX**. Same posture as UART / SPI.
- **ESP32 / RP2040 / AVR-DA I2C feature parity.** Gated on the
  in-flight kernel-clock-traits + irq-vector-traits work in
  alloy-codegen reaching those families.
- **Hardware spot-checks for the new levers.** Land per board with
  the existing matrix in a follow-up
  `validate-i2c-coverage-on-3-boards`.

## Alternatives Considered

**Generate the entire I2C HAL from the schema id.** Same posture as
ADC/UART/SPI. Rejected for the same reason: schema-aware code
violates the runtime/device boundary.

**Lock `set_clock_speed` to the closed `SpeedMode` enum.** Rejected
— SPI took the `set_clock_speed(u32 hz)` route with realisation
validation, and I2C should match for consistency. The
`SpeedMode` enum stays as a typed convenience preset.
