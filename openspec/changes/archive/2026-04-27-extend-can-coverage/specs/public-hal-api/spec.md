# public-hal-api Spec Delta: CAN Coverage Extension

## ADDED Requirements

### Requirement: CAN HAL SHALL Expose Data-Rate Timing And Test Modes Per Capability

The `alloy::hal::can::handle<P>` MUST expose
`set_data_timing(prescaler, sjw, tseg1, tseg2)` whenever
`kHasFlexibleDataRate`. The HAL MUST also expose
`enum class TestMode { Normal, BusMonitor, LoopbackInternal,
LoopbackExternal, Restricted }` plus `set_test_mode(TestMode)`
gated on the descriptor's test / bus-monitor field. Backends
without a mode MUST return `core::ErrorCode::NotSupported`.

#### Scenario: SAM-E70 MCAN runs FD at 2 Mbit/s

- **WHEN** an application calls
  `can.set_data_timing(prescaler, sjw, tseg1, tseg2)` chosen for
  2 Mbit/s on MCAN0 of `same70_xplained`
- **THEN** the call succeeds and the data-rate timing register
  is programmed
- **AND** subsequent `transmit(frame)` with `bit_rate_switch =
  true` runs at 2 Mbit/s

### Requirement: CAN HAL SHALL Expose Frame Primitives And Filter Banks

The HAL MUST expose `struct CanFrame` (id, extended_id,
remote_frame, fd_format, bit_rate_switch, dlc, data array
sized 64), plus `transmit(CanFrame)` and
`receive(CanFrame& out, FifoId fifo)` returning whether a frame
was retrieved.

The HAL MUST expose
`enum class FilterTarget { Rxfifo0, Rxfifo1, RejectMatching,
AcceptMatching }` plus
`set_filter_standard(std::uint8_t bank, std::uint16_t id,
std::uint16_t mask, FilterTarget)` and
`set_filter_extended(std::uint8_t bank, std::uint32_t id,
std::uint32_t mask, FilterTarget)`. Out-of-range banks MUST
return `core::ErrorCode::InvalidArgument`.

#### Scenario: Two filter banks split traffic between FIFO0 and FIFO1

- **WHEN** an application calls
  `can.set_filter_standard(0, 0x100, 0x7F0, FilterTarget::Rxfifo0)`
  then
  `can.set_filter_extended(1, 0x18FE'F100, 0x1FFF'FF00, FilterTarget::Rxfifo1)`
- **THEN** matching standard frames in 0x100..0x10F land in FIFO0
- **AND** matching extended frames in 0x18FE'F100..0x18FE'F1FF land
  in FIFO1

### Requirement: CAN HAL SHALL Expose Error Counters And Bus-Off Recovery

The HAL MUST expose:

- `tx_error_count() -> std::uint8_t`
- `rx_error_count() -> std::uint8_t`
- `bus_off() -> bool`
- `error_passive() -> bool`
- `error_warning() -> bool`

(All gated on `kErrorCounterRegister.valid`.)

The HAL MUST expose
`request_bus_off_recovery() -> Result<void, ErrorCode>` initiating
the 128 × 11 recessive-bit CAN-spec recovery sequence. Recovery is
NOT automatic — applications retain agency over when to rejoin.

#### Scenario: Bus-off detected after 256 TX errors

- **WHEN** the bus enters error-active → error-passive → bus-off
  states
- **THEN** `can.bus_off()` returns `true` once the transition lands
- **AND** `can.request_bus_off_recovery()` initiates the recovery
  sequence
- **AND** subsequent `bus_off()` returns `false` once the recovery
  completes

### Requirement: CAN HAL SHALL Expose Typed Interrupt Setters And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { Tx, RxFifo0, RxFifo1, Error, BusOff,
ErrorWarning, ErrorPassive, Wakeup }` plus
`enable_interrupt(InterruptKind)` /
`disable_interrupt(InterruptKind)` (each kind gated on its IE
field), and `irq_numbers() -> std::span<const std::uint32_t>`.

#### Scenario: Wakeup interrupt is gated on the wakeup field

- **WHEN** an application calls
  `can.enable_interrupt(InterruptKind::Wakeup)` on a peripheral
  whose wakeup IE field is invalid
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Async CAN Adapter SHALL Add wait_for And receive_frame

The runtime `async::can` namespace MUST expose
`wait_for<P>(handle, InterruptKind)` plus
`receive_frame<P>(handle, FifoId fifo) ->
core::Result<operation<…>, core::ErrorCode>` whose awaiter result
contains the received frame. Existing blocking APIs are unchanged.

#### Scenario: Coroutine receives a frame from FIFO0

- **WHEN** a task awaits
  `async::can::receive_frame<MCAN0>(can, FifoId::Fifo0)` while a
  frame matching a configured filter arrives on the bus
- **THEN** the task resumes when the FIFO0 interrupt fires and the
  awaiter's result contains the frame
