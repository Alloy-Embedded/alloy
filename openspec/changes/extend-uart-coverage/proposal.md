# Extend UART Coverage To Match Published Descriptor Surface

## Why

The latest alloy-devices publication ships an extremely rich
`UartSemanticTraits<P>` across every foundational ST / Microchip /
NXP device: 60+ typed register / field refs covering every CR1 / CR2
/ CR3 / BRR / ISR / RDR / TDR bit, plus Tier 2/3/4 metadata
(`kFifoDepth`, `kBaudOversamplingOptions`, `kBaudClockSourceRaw`,
`kFifoTriggerFractionsQ8`, `kSupportsLin`, `kDmaBindings`,
`kIrqNumbers`, `kKernelClockSelectorField`,
`kKernelClockSourceOptions`, `kKernelMaxClockHz`).

The runtime currently consumes ~10% of that surface: the HAL accepts
a `UartConfig{baudrate, data_bits, parity, stop_bits, flow_control,
peripheral_clock_hz}` and exposes `configure / write / write_byte /
read / flush / configure_tx_dma / configure_rx_dma / write_dma /
read_dma`. Baudrate is even constrained to a closed enum of 8
common rates. The remaining 90% — granular FIFO control, kernel-clock
source selection, LIN break detection, RS-485 driver-enable, smartcard
mode, IrDA, half-duplex single-wire, multiprocessor (9-bit address
mark), wake-up from STOP, oversampling 16/8 selection, all the
status flags an interrupt handler actually needs — sits unused.

modm sets the bar for what "complete UART" looks like in C++ on
embedded: every register bit lifted into a typed setter, all
peripherals reach the same surface, the type system rejects
unsupported combinations at compile time. Alloy already publishes the
data needed; this change is plumbing.

Concrete scope: lift every published `UartSemanticTraits<P>` field
and Tier 2/3/4 array into typed HAL methods, capability-gated via
`if constexpr` against `semantic_traits::k<…>.valid` /
`kSupports<…>` / `kHas<…>` flags. Wire the new IRQ + DMA + kernel-
clock-source levers into the existing `async::uart::write_dma` /
`read_dma` from `complete-async-hal`. Ship a `uart_probe_complete`
example exercising every lever. Document in `docs/UART.md`.

## What Changes

### `src/hal/uart/uart.hpp` — extended HAL surface (additive only)

New methods on `port_handle<Connector>`. Every method
capability-gated via `if constexpr` against the published trait
field's `.valid` flag (or a Tier-2/3/4 capability bool / non-empty
array). Unsupported levers return
`core::ErrorCode::NotSupported`.

- **Baudrate (raw u32, no enum lock-in)**
  - `set_baudrate(std::uint32_t bps)` — computes BRR from
    `peripheral_clock_hz` + chosen oversampling. The current closed
    `Baudrate` enum stays as the convenience preset; raw u32
    augments it.
  - `set_oversampling(Oversampling)` — `X16` / `X8`; gated on
    `kBaudOversamplingOptions.size() > 1`.
  - `kernel_clock_hz() -> u32` — reads back the resolved kernel
    clock, useful for asserts.
- **Frame**
  - Existing `data_bits`, `parity`, `stop_bits` keep working.
  - `set_msb_first(bool)` — gated on the descriptor's MSBFIRST
    field where present.
  - `set_invert_polarity(rx, tx, data)` — three independent inversion
    knobs (DATAINV, RXINV, TXINV in CR2 on STM32).
- **FIFO (gated on `kFifoDepth > 0`)**
  - `enable_fifo(bool)`
  - `set_tx_threshold(FifoTrigger)` / `set_rx_threshold(FifoTrigger)`
    — `FifoTrigger` is `Empty`, `Quarter`, `Half`, `ThreeQuarters`,
    `Full` per the descriptor's published `kFifoTriggerFractionsQ8`
    set. The HAL clamps requests to the published set.
  - `tx_fifo_full() -> bool`, `rx_fifo_empty() -> bool`,
    `rx_fifo_threshold_reached() -> bool`.
- **Kernel clock source (gated on `kKernelClockSourceOptions` non-empty)**
  - `set_kernel_clock_source(KernelClockSource)` — typed enum
    derived from descriptor's `KernelClockSourceOption` array
    (`pclk2`, `sysclk`, `hsi16`, `lse`).
- **LIN (gated on `kSupportsLin`)**
  - `enable_lin(bool)`, `send_lin_break()`, `lin_break_detected() -> bool`,
    `clear_lin_break_flag()`.
- **RS-485 driver enable (DE)**
  - `enable_de(bool)`, `set_de_assertion_time(u8 sample_times)`,
    `set_de_deassertion_time(u8)` — gated on the DE field group.
- **Half-duplex single-wire**
  - `set_half_duplex(bool)` — gated on HDSEL.
- **Smartcard / IrDA (gated separately)**
  - `set_smartcard_mode(bool)` / `set_irda_mode(bool)` — descriptor
    fields exist; HAL exposes them gated.
- **Multiprocessor (9-bit address mark)**
  - `set_address(u8 addr, AddressLength len)`, `mute_until_address(bool)`.
- **Wake from STOP**
  - `enable_wakeup_from_stop(WakeupTrigger)` — typed enum
    `AddressMatch`, `RxneNonEmpty`, `StartBit`. Gated on UESM.
- **Status flag accessors (typed)**
  - `tx_complete() -> bool`, `tx_register_empty() -> bool`,
    `rx_register_not_empty() -> bool`, `parity_error() -> bool`,
    `framing_error() -> bool`, `noise_error() -> bool`,
    `overrun_error() -> bool`, `clear_*` for each.
- **Interrupts (typed enable/disable)**
  - `enable_interrupt(InterruptKind)` /
    `disable_interrupt(InterruptKind)` — `InterruptKind` covers
    `Tc`, `Txe`, `Rxne`, `IdleLine`, `LinBreak`, `Cts`, `Error`,
    `RxFifoThreshold`, `TxFifoThreshold`. Each gated on the
    corresponding control-side field.
- **NVIC vector lookup**
  - `static constexpr auto irq_numbers() -> std::span<const u32>` —
    returns `kIrqNumbers` from the descriptor so application code
    can install ISRs without hardcoding NVIC line ids.

### Per-peripheral typed `BaudClockSource` enum

Codegen extension (paired alloy-codegen change, see Out of Scope):
each emitted `UartSemanticTraits<P>` will gain a typed
`UartClockSourceOf<P>::type` enum mirroring the AdcChannelOf pattern
landed in `add-adc-channel-typed-enum`. Until that change lands the
HAL accepts a raw `KernelClockSource` enum (already published).

### Async integration

Existing `async::uart::write_dma / read_dma` stay; the new IRQ-vector
lookup means an example can self-install its `Reset_Handler`-time
ISR table without per-board code.

`async::uart::wait_for(InterruptKind)` is added as a sibling to the
DMA wrappers — for polling-free wait on idle-line, address-match,
or LIN-break events.

### `examples/uart_probe_complete/`

Targets `nucleo_g071rb` USART1. Configures:
- 921600 8N1, FIFO enabled, RX threshold = Quarter
- LIN auto-detect on USART2
- RS-485 DE on USART3 with 1-bit assertion / 1-bit deassertion
- Wake-from-STOP on USART4 listening for address 0x42

Mirrors a modm `Uart` recipe with each lever.

### Docs

`docs/UART.md` — comprehensive guide. Replaces the scattered UART
mentions in cookbook fragments. Sections: model, baudrate / FIFO /
kernel-clock recipes, LIN / RS-485 / smartcard recipes, error
handling, async wiring, modm migration table.

## What Does NOT Change

- Existing `UartConfig` / `configure` / `write` / `read` / DMA APIs
  are unchanged. New methods are additive on `port_handle<C>`.
- UART tier in `docs/SUPPORT_MATRIX.md` stays `foundational` —
  hardware spot-checks for new levers land per board with the
  existing 3-board matrix (SAME70 / STM32G0 / STM32F4).
- Descriptor-side fields are NOT changed in this proposal — they're
  already published and stable.

## Out of Scope (Follow-Up Changes)

- **`add-uart-clock-source-typed-enum` (alloy-codegen).** Mirrors
  the `add-adc-channel-typed-enum` pattern: emits
  `UartClockSourceOf<P>::type` per peripheral so the HAL drops its
  raw `KernelClockSource` enum and consumes typed values directly.
  Tracked separately because it pairs with the codegen
  `add-kernel-clock-traits` work that just landed.
- **ESP32 / RP2040 / AVR-DA UART coverage.** Descriptors with
  `kPresent=true` UART get the same surface; the in-flight kernel-
  clock-traits and irq-vector-traits work in alloy-codegen needs to
  reach those families before this change applies meaningfully to
  them.
- **DMA double-buffer / circular RX**. The descriptor publishes the
  DMA bindings; the runtime side already supports circular through
  `enable_dma(circular=true)` on the DMA channel. A typed
  "ring-buffer-and-IDLE-interrupt" recipe (the canonical "receive
  unknown-length frame" idiom) ships as a follow-up
  `add-uart-idle-line-rx-dma` example.
- **9-bit data with parity quirks**. ST USART has the M0/M1 pair
  encoding 7/8/9-bit; some configs are nonsensical. The HAL accepts
  whatever the user passes; a future
  `add-uart-validate-frame-config` change introduces the negative
  matrix.
- **Hardware spot-checks for the new levers**. Land per board with
  the existing `uart_logger` / `uart_console` matrix in a follow-up
  `validate-uart-coverage-on-3-boards`.

## Alternatives Considered

**Generate the entire UART HAL from the schema id.** Each schema
(`schema_alloy_uart_st_sci3_v2_1_cube`, `…_microchip_usart_v3`,
`…_nxp_lpuart_imxrt`) could drive a per-vendor template. Rejected —
schema-aware code violates the runtime/device boundary; the runtime
reaches for typed register / field refs, not vendor peripheral type
names. The capability-flag + indexed-field approach already absorbs
every difference.

**Keep the closed `Baudrate` enum.** Rejected — the enum has 8
values and 921600+ baud rates are common in real applications. Raw
u32 is the right shape; the enum stays as a convenience preset.

**Wait for ESP32 / RP2040 / AVR-DA UART traits to fill out before
extending.** Rejected — gating ST/Microchip/NXP coverage on every
family blocks the 3-board hardware matrix that's already
foundational. The capability gates already produce `NotSupported`
cleanly when a trait is absent.
