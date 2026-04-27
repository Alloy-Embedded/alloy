# public-hal-api Spec Delta: SPI Coverage Extension

## ADDED Requirements

### Requirement: SPI HAL SHALL Accept Variable Data Size 4-16 With Capability Validation

The `alloy::hal::spi::port_handle<C>` MUST expose
`set_data_size(std::uint8_t bits)` whenever
`kDsField.valid || kDffField.valid`. The call MUST validate the
requested width against the published field's encoding range:
- `kDsField` (4 bits wide on STM32 G0/F4) accepts 4–16.
- `kDffField` (1 bit wide on legacy STM32) accepts only 8 / 16.

Out-of-range requests MUST return
`core::ErrorCode::InvalidArgument`. Compile-time-constant requests
outside `[4, 16]` MUST `static_assert`.

#### Scenario: STM32G0 SPI1 supports 12-bit frames

- **WHEN** an application calls `spi.set_data_size(12)` on SPI1 of
  `nucleo_g071rb` (kDsField is 4 bits wide)
- **THEN** the call succeeds and DS is programmed to encode 12 bits

#### Scenario: 12-bit on a kDffField-only peripheral is rejected

- **WHEN** an application calls `spi.set_data_size(12)` on a
  legacy peripheral whose only width control is `kDffField`
  (1 bit, 8 or 16 only)
- **THEN** the call returns `core::ErrorCode::InvalidArgument`
- **AND** no MMIO write occurs

### Requirement: SPI HAL SHALL Accept Raw Clock Speed With ±5% Realisation Validation

The HAL MUST expose `set_clock_speed(std::uint32_t hz)` resolving
the BR divider from the peripheral kernel clock + the published
BR field encoding. The call MUST return
`core::ErrorCode::InvalidArgument` when the realised rate falls
outside ±5 % of the requested rate. A sibling
`realised_clock_speed() -> std::uint32_t` accessor MUST be
available so callers can inspect what the BR encoding produces
without committing.

#### Scenario: 16 MHz request on STM32G0 succeeds with realised rate within ±5 %

- **WHEN** an application calls `spi.set_clock_speed(16'000'000u)`
  on SPI1 of `nucleo_g071rb` with the default 64 MHz kernel clock
- **THEN** the call succeeds, BR is programmed to /4, and
  `realised_clock_speed()` returns 16 MHz

#### Scenario: 5 MHz request on STM32G0 fails (powers-of-2 only)

- **WHEN** an application calls `spi.set_clock_speed(5'000'000u)` on
  SPI1 with the default 64 MHz kernel clock — the realisable rates
  are 32 / 16 / 8 / 4 / 2 / 1 MHz, none within ±5 % of 5 MHz
- **THEN** the call returns `core::ErrorCode::InvalidArgument`
- **AND** the BR field is unchanged

### Requirement: SPI HAL SHALL Expose Frame-Format And CRC Setters Per Capability

The HAL MUST expose:

- `enum class FrameFormat { Motorola, TI }`,
  `set_frame_format(FrameFormat)` — gated independently on
  `kSupportsMotorolaFrame` / `kSupportsTiFrame`.
- CRC (gated on `kSupportsCrc`):
  `enable_crc(bool)`,
  `set_crc_polynomial(std::uint16_t)`,
  `read_crc() -> std::uint16_t`,
  `crc_error() -> bool`,
  `clear_crc_error() -> Result<void, ErrorCode>`.

Backends without the capability MUST return
`core::ErrorCode::NotSupported` for every method in that group.

#### Scenario: STM32 SPI1 supports CCITT polynomial 0x1021

- **WHEN** an application calls `spi.enable_crc(true)` then
  `spi.set_crc_polynomial(0x1021u)` on SPI1 of `nucleo_g071rb`
- **THEN** both calls succeed
- **AND** subsequent transfers append the CRC and `read_crc()`
  returns the computed value

#### Scenario: SAM SPI rejects CRC because kSupportsCrc is false

- **WHEN** an application calls `spi.enable_crc(true)` against a
  SAME70 SPI peripheral whose `kSupportsCrc` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: SPI HAL SHALL Expose Bidirectional 3-Wire And Hardware NSS Management

The HAL MUST expose, each gated independently:

- Bidirectional 3-wire (`kSupportsBidirectional3Wire`):
  `set_bidirectional(bool)`,
  `enum class BiDir { Receive, Transmit }`,
  `set_bidirectional_direction(BiDir)`.
- Hardware NSS management (`kSupportsNssHwManagement`):
  `enum class NssManagement { Software, HardwareInput, HardwareOutput }`.
  `set_nss_management(NssManagement)`,
  `set_nss_pulse_per_transfer(bool)`.

Backends without the capability MUST return
`core::ErrorCode::NotSupported`.

#### Scenario: STM32 SPI drives NSS with NSSP pulse per transfer

- **WHEN** an application calls
  `spi.set_nss_management(NssManagement::HardwareOutput)` then
  `spi.set_nss_pulse_per_transfer(true)` on SPI1 of `nucleo_g071rb`
- **THEN** both calls succeed
- **AND** SSOE is set, SSM is cleared, and NSSP is set; subsequent
  transfers pulse NSS between consecutive frames

### Requirement: SPI HAL SHALL Expose SAM-Style Per-CS Timing Where Published

The HAL MUST expose, gated on the corresponding descriptor field:

- `set_cs_decode_mode(bool)` — gated on `kPcsdecField.valid`.
- `set_cs_delay_between_consecutive(std::uint16_t cycles)` —
  through `kDlybctField`.
- `set_cs_delay_clock_to_active(std::uint16_t cycles)` — through
  `kDlybsField`.
- `set_cs_delay_active_to_clock(std::uint16_t cycles)` — through
  `kDlybcsField`.

Cycle counts are raw peripheral-clock cycles; applications convert
their microsecond requirement using `kernel_clock_hz()`.

#### Scenario: SAME70 SPI configures 100-cycle delay between transfers

- **WHEN** an application calls
  `spi.set_cs_delay_between_consecutive(100u)` on SAME70 SPI0
- **THEN** the call succeeds and the corresponding CSR.DLYBCT
  field is programmed with the requested cycle count

#### Scenario: STM32 SPI rejects per-CS delay setters

- **WHEN** an application calls
  `spi.set_cs_delay_between_consecutive(100u)` on STM32G0 SPI1
  (`kDlybctField.valid == false`)
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: SPI HAL SHALL Expose Typed Status Flag And Interrupt Setters

The HAL MUST expose:

- Typed status accessors: `tx_register_empty()`,
  `rx_register_not_empty()`, `busy()`, `mode_fault()`,
  `frame_format_error()` — each gated on the corresponding
  descriptor field; each with a `clear_*` mirror where the
  descriptor publishes a clear-side field.
- `enum class InterruptKind { Txe, Rxne, Error, ModeFault,
  CrcError, FrameError }`.
- `enable_interrupt(InterruptKind)` /
  `disable_interrupt(InterruptKind)` — each kind gated on the
  corresponding control-side IE field.

Unsupported kinds MUST return `core::ErrorCode::NotSupported`.

#### Scenario: ModeFault is observable and clearable on STM32 SPI

- **WHEN** an external master pulls NSS low while the peripheral
  is configured as master
- **THEN** `spi.mode_fault()` returns `true`
- **AND** `spi.clear_mode_fault()` returns `Ok` and the next
  `mode_fault()` call returns `false`

#### Scenario: CrcError interrupt is gated on kSupportsCrc

- **WHEN** an application calls
  `spi.enable_interrupt(InterruptKind::CrcError)` on a peripheral
  whose `kSupportsCrc` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: SPI HAL SHALL Expose The Per-Peripheral IRQ Number List

The HAL MUST expose
`static constexpr auto port_handle<C>::irq_numbers() ->
std::span<const std::uint32_t>` returning the descriptor's
`kIrqNumbers` for the bound peripheral, mirroring the UART
contract.

#### Scenario: STM32G0 SPI1 advertises one IRQ line

- **WHEN** an application reads `decltype(spi)::irq_numbers()` on
  SPI1 of `nucleo_g071rb`
- **THEN** the span has size 1 and contains the SPI1 NVIC IRQ
  number published by the descriptor

### Requirement: Async SPI Adapter SHALL Add wait_for(InterruptKind)

The runtime `async::spi` namespace MUST expose
`wait_for<P>(port, InterruptKind kind) ->
core::Result<operation<…>, core::ErrorCode>` so a coroutine can
`co_await` an Rxne, ModeFault, or CrcError event without polling.
The default existing wrappers (`write_dma` / `read_dma` /
`transfer_dma`) are unchanged.

#### Scenario: Coroutine wakes on CRC error

- **WHEN** a task awaits
  `async::spi::wait_for<SPI1>(spi, InterruptKind::CrcError)` while
  a transfer with CRC enabled is in flight
- **THEN** the task resumes when the CRC error interrupt fires
- **AND** the awaiter returns `Ok`
