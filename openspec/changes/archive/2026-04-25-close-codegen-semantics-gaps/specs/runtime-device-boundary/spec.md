## ADDED Requirements

### Requirement: Runtime Contract Shall Emit Semantic FieldRef For Every HAL-Referenced Register Field

The generated runtime contract SHALL emit a fully-populated `RuntimeFieldRef`
for every CR / SR / MR / MAN / CFGR bit that the public HAL backends
reference when driving a peripheral. `kInvalidFieldRef` SHALL NOT appear in
the `driver_semantics/*.hpp` entries that the HAL declares as dependencies.

#### Scenario: TWIHS START / STOP bits resolve to valid FieldRef

- **WHEN** a board selects a SAM E70 device and the HAL includes
  `driver_semantics/i2c.hpp`
- **THEN** `kStartField`, `kStopField`, `kMsenField`, `kMsdisField`, and
  `kSvdisField` each resolve to a valid `RuntimeFieldRef` (non-`kInvalidFieldRef`)
  with correct bit offset and width
- **AND** the TWIHS backend can drive START / STOP without raw bit masks

#### Scenario: Semantic smoke test catches a regression

- **WHEN** the generator is modified and accidentally emits
  `kInvalidFieldRef` for a field the HAL references
- **THEN** the `contract_smoke.cmake` semantic-completeness check fails at
  build time before the regression lands

### Requirement: Runtime Contract Shall Emit Typed Pin-Route Helper

The generated runtime contract SHALL expose a typed `alloy::pinmux::route`
helper that consumes the `(PinId, PeripheralId, SignalId)` triples already
captured in `routes.hpp` and writes the vendor-correct selector registers
(`ABCDSR1` / `ABCDSR2` / `PDR` on SAM E70; `AFR` on STM32; `IOCON` on NXP).

#### Scenario: Adopter muxes a PHY pin through the typed helper

- **WHEN** user code calls
  `alloy::pinmux::route<PinId::PD0, PeripheralId::GMAC, SignalId::signal_gtxck>()`
- **THEN** the helper writes the peripheral-A selector (value `0`) into
  `PIOD.ABCDSR1` / `ABCDSR2` at pin offset 0 and releases PIOD bit 0 to
  peripheral control via `PDR`
- **AND** no MMIO literal appears in user code

#### Scenario: Invalid route fails at compile time

- **WHEN** user code requests a `(Pin, Peripheral, Signal)` triple that does
  not exist in the contract
- **THEN** compilation fails with a diagnostic that names the missing route

### Requirement: Runtime Contract Shall Emit Clock-Gate Enable / Disable Helpers

The generated runtime contract SHALL expose
`alloy::clock::enable(PeripheralId)` and `alloy::clock::disable(PeripheralId)`
free functions that resolve to the correct clock-gate register write for the
selected device, using the `ClockGateId` + `clock_bindings.hpp` artifacts
already emitted.

#### Scenario: Adopter enables the GMAC peripheral clock

- **WHEN** user code calls `alloy::clock::enable(PeripheralId::GMAC)` on a
  SAM E70 device
- **THEN** the helper writes bit 7 (PID 39 - 32) into PMC_PCER1
- **AND** the adopter does not need to know the PID number or the register
  address

### Requirement: Runtime Contract Shall Emit Typed Peripheral Base Accessor

The generated runtime contract SHALL expose
`template <PeripheralId Id> constexpr auto alloy::device::base()` returning
`PeripheralInstanceTraits<Id>::kBaseAddress`. HAL backends and probes SHALL
use this accessor instead of typing base-address literals.

#### Scenario: Probe uses typed base for GMAC

- **WHEN** `examples/driver_ksz8081_probe/main.cpp` accesses GMAC registers
- **THEN** it obtains the base from
  `alloy::device::base<PeripheralId::GMAC>()`, which returns `0x40050000u`
  on SAM E70
- **AND** no numeric base-address literal appears in the probe source

### Requirement: HAL Shall Expose GPIO Configuration For Non-Peripheral Pins

The public HAL SHALL allow configuring a pin as push-pull output for use
cases that have no associated peripheral signal (external peripheral reset
lines, LEDs, strap probes) without needing a `PeripheralId` / `SignalId`
binding.

#### Scenario: PHY reset line is driven through the HAL

- **WHEN** a probe needs to toggle the KSZ8081 nRESET line on PC10
- **THEN** it calls `alloy::hal::gpio::configure(PinId::PC10, Direction::Output)`
  + `drive_high()` / `drive_low()`
- **AND** no raw `SODR` / `CODR` / `OER` register write appears in the
  probe source
