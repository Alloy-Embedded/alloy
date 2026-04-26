# public-hal-api Spec Delta: UART Coverage Extension

## ADDED Requirements

### Requirement: UART HAL SHALL Accept Raw Baudrate Values With Range Validation

The `alloy::hal::uart::port_handle<C>` MUST expose
`set_baudrate(std::uint32_t bps)` resolving the BRR divisor from the
peripheral's current kernel clock and chosen oversampling. The call
MUST return `core::ErrorCode::InvalidArgument` when the resulting
BRR overflows 16 bits OR when the realised baud rate falls outside
±2 % of the requested rate. The closed `Baudrate` enum stays as a
typed convenience preset.

#### Scenario: 921600 baud is accepted on STM32G0 USART1

- **WHEN** an application calls `uart.set_baudrate(921600u)` on
  USART1 of `nucleo_g071rb` with the default 64 MHz kernel clock
- **THEN** the call succeeds and BRR is programmed to a value
  within ±2 % of the requested rate

#### Scenario: Out-of-range baudrate is rejected with InvalidArgument

- **WHEN** an application requests a baudrate that overflows the
  16-bit BRR (e.g. extremely low rate at high kernel clock) or that
  cannot be represented within ±2 %
- **THEN** the call returns `core::ErrorCode::InvalidArgument`
- **AND** the BRR register is not modified

### Requirement: UART HAL SHALL Expose Oversampling And Kernel-Clock-Source Setters

The HAL MUST expose `set_oversampling(Oversampling)` (with values
`X16` and `X8`) and `set_kernel_clock_source(KernelClockSource)`
whenever the descriptor publishes the corresponding traits
(`kBaudOversamplingOptions.size() > 1` for oversampling,
`kKernelClockSourceOptions` non-empty for clock source). Backends
without a published option set MUST return
`core::ErrorCode::NotSupported`.

#### Scenario: STM32G0 supports both 16x and 8x oversampling

- **WHEN** an application calls `uart.set_oversampling(Oversampling::X8)`
  on USART1 of `nucleo_g071rb`
- **THEN** the call succeeds and the OVER8 bit in CR1 is set

#### Scenario: Out-of-set kernel clock source is rejected

- **WHEN** an application requests a kernel clock source whose
  enumerator is not in the descriptor's
  `kKernelClockSourceOptions`
- **THEN** the call returns `core::ErrorCode::NotSupported`
- **AND** no MMIO write occurs

### Requirement: UART HAL SHALL Expose FIFO Control On Backends With kFifoDepth > 0

The HAL MUST expose `enable_fifo(bool)`,
`set_tx_threshold(FifoTrigger)`, `set_rx_threshold(FifoTrigger)`,
`tx_fifo_full()`, `rx_fifo_empty()`, and
`rx_fifo_threshold_reached()` whenever `kFifoDepth > 0` and the
matching control / status field-refs are valid. `FifoTrigger` is a
typed enum (`Empty`, `Quarter`, `Half`, `ThreeQuarters`, `Full`)
clamped to the descriptor's `kFifoTriggerFractionsQ8` set;
unsupported fractions return `core::ErrorCode::NotSupported`.

#### Scenario: STM32G0 USART1 supports the documented 5-fraction FIFO trigger set

- **WHEN** an application calls `uart.set_rx_threshold(FifoTrigger::Quarter)`
  on USART1 of `nucleo_g071rb`
- **THEN** the call succeeds and the RXFTCFG field in CR3 is
  programmed to encode 1/4

#### Scenario: Backend without a FIFO returns NotSupported

- **WHEN** an application calls `uart.enable_fifo(true)` against a
  peripheral whose `kFifoDepth` is 0 (e.g. classic STM32F1 USART)
- **THEN** the call returns `core::ErrorCode::NotSupported`
- **AND** no MMIO write occurs

### Requirement: UART HAL SHALL Expose Typed Status-Flag And Interrupt Setters

The HAL MUST expose typed status-flag accessors
(`tx_complete`, `tx_register_empty`, `rx_register_not_empty`,
`parity_error`, `framing_error`, `noise_error`, `overrun_error`,
each with a `clear_*` mirror where the descriptor publishes a
clear-side field) and typed interrupt-enable / disable methods
(`enable_interrupt(InterruptKind)` / `disable_interrupt(InterruptKind)`
with `InterruptKind` covering `Tc`, `Txe`, `Rxne`, `IdleLine`,
`LinBreak`, `Cts`, `Error`, `RxFifoThreshold`, `TxFifoThreshold`).
Each accessor and each interrupt kind is gated on the corresponding
descriptor field; unsupported kinds return
`core::ErrorCode::NotSupported`.

#### Scenario: Overrun flag is observable and clearable

- **WHEN** the application leaves an RX byte unread long enough that
  the descriptor's overrun field flips
- **THEN** `uart.overrun_error()` returns `true`
- **AND** `uart.clear_overrun_error()` returns `Ok` and the next
  `overrun_error()` returns `false`

#### Scenario: LinBreak interrupt is gated on LIN support

- **WHEN** an application calls
  `uart.enable_interrupt(InterruptKind::LinBreak)` on a backend
  whose `kSupportsLin` is `false`
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: UART HAL SHALL Expose The Per-Peripheral IRQ Number List

The HAL MUST expose
`static constexpr auto port_handle<C>::irq_numbers() ->
std::span<const std::uint32_t>` returning the descriptor's
`kIrqNumbers` for the bound peripheral. Application code uses this
to install ISRs without hardcoding NVIC line ids.

#### Scenario: STM32G0 USART1 advertises one IRQ line

- **WHEN** an application reads
  `decltype(uart)::irq_numbers()` on USART1 of `nucleo_g071rb`
- **THEN** the span has size 1 and contains the value 27 (the
  STM32G0 USART1 NVIC IRQ number)

#### Scenario: SAME70 USART advertises multiple IRQ lines

- **WHEN** an application reads
  `decltype(uart)::irq_numbers()` on a SAME70 USART peripheral
  whose descriptor publishes a multi-IRQ surface
- **THEN** the span enumerates each line in publication order
- **AND** the values match the descriptor's `kIrqNumbers` array
  byte-for-byte

### Requirement: UART HAL SHALL Expose LIN / RS-485-DE / Half-Duplex / Smartcard / IrDA / Multiprocessor / Wakeup Setters Per Capability

The HAL MUST expose, each gated independently on the corresponding
descriptor field group:

- LIN (`kSupportsLin`): `enable_lin(bool)`, `send_lin_break()`,
  `lin_break_detected() -> bool`, `clear_lin_break_flag()`.
- RS-485 driver enable: `enable_de(bool)`,
  `set_de_assertion_time(std::uint8_t)`,
  `set_de_deassertion_time(std::uint8_t)`.
- Half-duplex single-wire: `set_half_duplex(bool)`.
- Smartcard: `set_smartcard_mode(bool)`.
- IrDA: `set_irda_mode(bool)`.
- Multiprocessor: `set_address(std::uint8_t, AddressLength)`,
  `mute_until_address(bool)` (with `AddressLength` covering
  `Bits4` and `Bits7`).
- Wake-from-STOP: `enable_wakeup_from_stop(WakeupTrigger)` with
  `WakeupTrigger` covering `AddressMatch`, `RxneNonEmpty`,
  `StartBit`.

Backends whose descriptor lacks the corresponding field group MUST
return `core::ErrorCode::NotSupported` for every method in that
group.

#### Scenario: LPUART1 wakes the system on address match

- **WHEN** an application targets `nucleo_g071rb` LPUART1, calls
  `uart.set_address(0x42, AddressLength::Bits7)`, and
  `uart.enable_wakeup_from_stop(WakeupTrigger::AddressMatch)`
- **THEN** both calls succeed
- **AND** entering STOP and receiving a frame addressed to 0x42
  triggers the configured wake event

#### Scenario: USART4 rejects LIN because LIN is not supported on that peripheral

- **WHEN** an application calls `uart.enable_lin(true)` on USART4
  of `nucleo_g071rb` (LIN is supported on USART1/2/3 but not on
  USART4)
- **THEN** the call returns `core::ErrorCode::NotSupported`
- **AND** the LIN field group remains unchanged

### Requirement: Async UART Adapter SHALL Add wait_for(InterruptKind)

The runtime `async::uart` namespace MUST expose
`wait_for<P>(port, InterruptKind kind) ->
core::Result<operation<…>, core::ErrorCode>` so a coroutine can
`co_await` an idle-line, address-match, LIN-break, or other typed
interrupt event without polling. The default existing wrappers
(`write_dma` / `read_dma`) are unchanged; this is a sibling.

#### Scenario: Coroutine wakes on IDLE-line interrupt

- **WHEN** a task awaits
  `async::uart::wait_for<USART1>(uart, InterruptKind::IdleLine)`
  while a circular DMA RX is in flight
- **THEN** the task resumes when the IDLE-line interrupt fires,
  signalling end-of-frame for unknown-length receive
- **AND** the awaiter returns `Ok`
