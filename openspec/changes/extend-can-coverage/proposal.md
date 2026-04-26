# Extend CAN Coverage To Match Published Descriptor Surface

## Why

`CanSemanticTraits<P>` publishes the full classic + FD register
surface: nominal + data timing (prescaler / sync-jump-width /
time-segment-1 / time-segment-2), bit-rate switch enable, FD
operation enable, bus-monitor mode, init field, global filter
register, extended ID mask + filter config registers, error
counter register, interrupt enable / line-enable / line-select /
status registers, plus capability flag `kHasFlexibleDataRate`
and Tier 2/3/4 metadata (kIrqNumbers, kKernelClock).

The runtime currently consumes ~40%: HAL exposes
`configure / enter_init_mode / leave_init_mode /
enable_configuration / enable_fd_operation / enable_bit_rate_switch /
set_nominal_timing / enable_rx_fifo0_interrupt`. Data timing for
FD, filter banks (standard + extended), error counters, bus-off
detection / recovery, transceiver delay compensation, message
RAM addressing, bus-monitor / loop-back / restricted-operation
modes — none reachable today.

modm covers CAN end-to-end with classic + FD + filter banks +
error frame handling. Alloy already publishes the data needed;
this change is plumbing.

## What Changes

### `src/hal/can.hpp` — extended HAL surface (additive only)

- **Data-rate timing for FD** (gated on `kHasFlexibleDataRate`):
  `set_data_timing(prescaler, sjw, tseg1, tseg2)` —
  separate from the existing nominal timing.
- **Bus-monitor / loopback / restricted-operation modes**:
  `enum class TestMode { Normal, BusMonitor, LoopbackInternal,
  LoopbackExternal, Restricted }`.
  `set_test_mode(TestMode)` — gated on the descriptor's TEST or
  CCCR.MON field.
- **Filter banks** (gated on `kGlobalFilterRegister.valid` /
  `kExtendedFilterConfigRegister.valid`):
  `enum class FilterTarget { Rxfifo0, Rxfifo1, RejectMatching,
  AcceptMatching }`.
  `set_filter_standard(std::uint8_t bank, std::uint16_t id,
  std::uint16_t mask, FilterTarget)`,
  `set_filter_extended(std::uint8_t bank, std::uint32_t id,
  std::uint32_t mask, FilterTarget)`.
- **Error counters** (gated on `kErrorCounterRegister.valid`):
  `tx_error_count() -> std::uint8_t`,
  `rx_error_count() -> std::uint8_t`,
  `bus_off() -> bool`,
  `error_passive() -> bool`,
  `error_warning() -> bool`.
- **Bus recovery**:
  `request_bus_off_recovery() -> Result<void, ErrorCode>` —
  initiates the 128 × 11 recessive-bit recovery sequence.
- **Transmit / receive primitives**:
  `transmit(CanFrame) -> Result<TxHandle, ErrorCode>`,
  `receive(CanFrame& out, FifoId fifo = Fifo0) -> Result<bool,
  ErrorCode>` — `bool` is true if a frame was retrieved.
- **Typed interrupts**:
  `enum class InterruptKind { Tx, RxFifo0, RxFifo1, Error,
  BusOff, ErrorWarning, ErrorPassive, Wakeup }`.
  `enable_interrupt(InterruptKind)` /
  `disable_interrupt(InterruptKind)`.
- **NVIC vector lookup**: `irq_numbers() ->
  std::span<const std::uint32_t>`.
- **Async sibling**: `async::can::wait_for(InterruptKind)` plus
  `async::can::receive_frame(FifoId)` returning the frame in the
  awaiter result.

### `examples/can_probe_complete/`

Targets `same70_xplained` MCAN0. Configures classic 500 kbit/s
nominal + FD 2 Mbit/s data, two filter banks (standard + extended),
async receive task, bus-off recovery handler.

### Docs

`docs/CAN.md` — comprehensive guide: classic vs FD timing math,
filter bank programming, error counter semantics, bus-off recovery,
transceiver delay compensation, async wiring, modm migration table.

## What Does NOT Change

- Existing CAN API unchanged. Additive only.
- Tier stays `experimental` until hardware spot-check matrix lands.

## Out of Scope (Follow-Up Changes)

- Transceiver delay compensation (TDC) tuning interface — descriptor
  publishes the field; surface is a follow-up because TDC tuning
  varies wildly per transceiver.
- Hardware spot-checks → `validate-can-coverage-on-3-boards`.

## Alternatives Considered

`set_filter_*` taking raw bank index vs typed `FilterBank<P, n>`
template — going with raw `std::uint8_t` for now; a future
codegen `add-can-filter-bank-typed-enum` lifts to typed form.
