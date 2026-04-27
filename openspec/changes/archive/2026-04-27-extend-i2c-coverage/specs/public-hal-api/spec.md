# public-hal-api Spec Delta: I2C Coverage Extension

## ADDED Requirements

### Requirement: I2C HAL SHALL Accept Raw Clock Speed With ±5% Realisation Validation Plus SpeedMode Preset

The `alloy::hal::i2c::port_handle<C>` MUST expose
`set_clock_speed(std::uint32_t hz)` resolving the timing register
(TIMINGR / CCR / CWGR) from the peripheral kernel clock. Returns
`core::ErrorCode::InvalidArgument` if the realised rate falls
outside ±5 % of the requested rate. A typed convenience preset
`SpeedMode { Standard100kHz, Fast400kHz, FastPlus1MHz }` MUST also
be exposed; `FastPlus1MHz` MUST be gated on `kSupportsFastPlus`.

#### Scenario: 1 MHz Fast Plus on STM32G0 succeeds with TIMINGR realised within ±5 %

- **WHEN** an application calls
  `i2c.set_speed_mode(SpeedMode::FastPlus1MHz)` on I2C1 of
  `nucleo_g071rb`
- **THEN** TIMINGR is programmed to a documented Fast Plus pattern
  and the call returns `Ok`

#### Scenario: FastPlus on a backend without kSupportsFastPlus is rejected

- **WHEN** an application calls
  `i2c.set_speed_mode(SpeedMode::FastPlus1MHz)` on a peripheral
  whose `kSupportsFastPlus` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: I2C HAL SHALL Expose Addressing Mode And Own-Address Setters

The HAL MUST expose `enum class AddressingMode { Bits7, Bits10 }`,
`set_addressing_mode(AddressingMode)`, and
`set_own_address(std::uint16_t addr, AddressingMode mode)`. Backends
without 10-bit support MUST return `core::ErrorCode::NotSupported`
for `Bits10`. Backends that publish a second OAR field MUST also
expose `set_dual_address(std::uint16_t)`.

#### Scenario: STM32G0 I2C1 supports 10-bit addressing as master

- **WHEN** an application calls
  `i2c.set_addressing_mode(AddressingMode::Bits10)` then
  `i2c.write(0x3FF, payload)` on I2C1 of `nucleo_g071rb`
- **THEN** the call succeeds and the master frames a 10-bit start
  sequence with the documented header byte

#### Scenario: 10-bit on a 7-bit-only backend is rejected

- **WHEN** an application calls
  `i2c.set_addressing_mode(AddressingMode::Bits10)` against a
  peripheral whose published address field is 7 bits wide
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: I2C HAL SHALL Expose Clock Stretching And Status Flag Accessors

The HAL MUST expose `set_clock_stretching(bool enabled)` (gated on
the NOSTRETCH field) and typed status accessors:

- `nack_received() -> bool`
- `arbitration_lost() -> bool`
- `bus_error() -> bool`

Each MUST have a `clear_*` mirror where the descriptor publishes a
clear-side field.

#### Scenario: NACK is observable and clearable

- **WHEN** the master writes to an address with no responder
- **THEN** `i2c.nack_received()` returns `true`
- **AND** `i2c.clear_nack_received()` returns `Ok` and the next
  `nack_received()` returns `false`

### Requirement: I2C HAL SHALL Expose SMBus And PEC Setters Per Capability

The HAL MUST expose `enable_smbus(bool)`,
`set_smbus_role(SmbusRole)` (gated on `kSupportsSmbus`), and
`enable_pec(bool)`, `last_pec() -> std::uint8_t`,
`pec_error() -> bool`, `clear_pec_error()` (gated on
`kSupportsPec`). Backends without the capability MUST return
`NotSupported` for every method in that group.

#### Scenario: SMBus enable on a non-SMBus peripheral is rejected

- **WHEN** an application calls `i2c.enable_smbus(true)` on a
  peripheral whose `kSupportsSmbus` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: I2C HAL SHALL Expose Typed Interrupt Setters And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { Tx, Rx, Stop, Tc, AddrMatch, Nack,
BusError, ArbitrationLoss, Overrun, PecError, Timeout, SmbAlert }`
plus `enable_interrupt(InterruptKind) /
disable_interrupt(InterruptKind)`. Each kind MUST be gated on the
corresponding control-side IE field; unsupported kinds MUST return
`NotSupported`. The HAL MUST also expose
`irq_numbers() -> std::span<const std::uint32_t>` returning the
descriptor's `kIrqNumbers`.

#### Scenario: PecError interrupt is gated on kSupportsPec

- **WHEN** an application calls
  `i2c.enable_interrupt(InterruptKind::PecError)` on a peripheral
  whose `kSupportsPec` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Async I2C Adapter SHALL Add wait_for(InterruptKind)

The runtime `async::i2c` namespace MUST expose
`wait_for<P>(port, InterruptKind kind) ->
core::Result<operation<…>, core::ErrorCode>` so a coroutine can
`co_await` a Stop, AddrMatch, BusError, or other typed event. The
default existing wrappers (`write` / `read` / `write_read`) are
unchanged.

#### Scenario: Coroutine wakes on bus error

- **WHEN** a task awaits
  `async::i2c::wait_for<I2C1>(i2c, InterruptKind::BusError)` while
  a transfer is in flight and a bus error occurs
- **THEN** the task resumes when the bus-error interrupt fires and
  the awaiter returns `Ok`
