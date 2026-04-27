# Design: Extend UART Coverage

## Context

The runtime exposes the bare-minimum UART HAL (`configure / write /
read / DMA wrappers`). The descriptor publishes 60+ field refs plus
Tier 2/3/4 capability arrays (FIFO, IRQ, DMA, baud-clock options,
kernel-clock source/max). This document captures the design for
closing the gap without breaking the existing 3-board hardware
matrix and without crossing the runtime/device boundary.

## Goals

1. Match modm-class UART completeness (granular FIFO control,
   kernel-clock source selection, LIN, RS-485 DE, smartcard, IrDA,
   half-duplex, multiprocessor mode, wake from STOP, all status
   flags) on every device the descriptor declares UART for.
2. Stay schema-agnostic: the HAL reaches for typed
   `RuntimeRegisterRef` / `RuntimeFieldRef` plus the Tier-2/3/4
   capability arrays — never for vendor schema names.
3. Compose cleanly with `complete-async-hal`'s
   `async::uart::write_dma / read_dma` so an `idle-line +
   circular-DMA` recipe is one `co_await`.
4. Keep the existing API byte-compatible — every change is additive.

## Non-Goals

- ESP32 / RP2040 / AVR-DA UART feature parity. They get the same
  surface as the publication catches up; the HAL's capability gates
  already produce `NotSupported` cleanly when traits are absent.
- 9-bit-data + parity validation matrix. Application sends what it
  sends; we don't pre-emptively reject every nonsensical
  combination. Tracked as
  `add-uart-validate-frame-config` follow-up.
- DMA double-buffer ring + IDLE-line "unknown-length frame" recipe
  as a built-in HAL method. Lands as an example
  (`uart_idle_line_rx_dma`) once this change is in.

## Key Decisions

### Decision 1: Capability gates via `if constexpr` on `.valid`/array-size

Same pattern as ADC. Every published trait carries either a `.valid`
bool (for `RuntimeFieldRef` / `RuntimeRegisterRef`) or a non-empty
array (for Tier 2/3/4 metadata). The HAL uses
`if constexpr (semantic_traits::kFooField.valid)` /
`if constexpr (!semantic_traits::kBarOptions.empty())` to either
emit the real implementation or fall through to
`core::ErrorCode::NotSupported`. The compiler eliminates the unused
branch entirely — no runtime cost on backends that don't support
the feature.

### Decision 2: Raw `u32 bps` for `set_baudrate`, keep `Baudrate` enum as preset

Real-world applications need 460800, 921600, 1000000, 2000000,
4000000 — values not in the closed enum. The closed enum stays as
a typed convenience preset. The new `set_baudrate(u32)` resolves
BRR from the runtime-resolved kernel clock + the chosen oversampling.

The HAL **rejects** baudrates that the BRR can't represent within
±2 % of the requested rate (BRR is a 16-bit divider; tight
sampling); it returns `core::ErrorCode::InvalidArgument` in that
case rather than silently programming a useless rate.

### Decision 3: `FifoTrigger` as typed enum tied to descriptor

The descriptor's `kFifoTriggerFractionsQ8` array publishes the
supported thresholds as Q8 fractions (e.g.
`{32u, 64u, 128u, 192u, 224u, 256u}` for `1/8`, `1/4`, `1/2`,
`3/4`, `7/8`, full). The HAL's `FifoTrigger` enum maps to the
five canonical fractions in the catalog. Backends with fewer
options (e.g. only quarter / half) return `NotSupported` for the
unsupported entries.

A future codegen extension can promote
`kFifoTriggerFractionsQ8` to a per-peripheral `UartFifoTriggerOf<P>`
typed enum (mirroring `AdcChannelOf<P>`); for now the HAL ships
the canonical 5-value enum and clamps.

### Decision 4: `KernelClockSource` enum reuses descriptor's enum

The descriptor's `KernelClockSourceOption` already defines a typed
enum (`pclk2`, `sysclk`, `hsi16`, `lse`, ...). The HAL imports it
directly via `using KernelClockSource = device::KernelClockSource;`.
Per-peripheral subset is enforced at runtime by checking the
caller's choice against the published `kKernelClockSourceOptions`
list — out-of-set returns `NotSupported`.

A future codegen change (`add-uart-clock-source-typed-enum`) will
emit `UartClockSourceOf<P>::type` per peripheral so the type system
can reject "this UART doesn't support LSE" at compile time. Until
then, runtime `NotSupported`.

### Decision 5: LIN / RS-485 / smartcard / IrDA / half-duplex are independent flags

Each has its own field group in the descriptor. The HAL exposes
each as its own setter and capability gate. The features are
mostly orthogonal — the HAL doesn't enforce mutual exclusion (the
descriptor's field gates already do, and conflicting writes flow
through to whatever the hardware does, which on STM32 is "last
write wins"). Documenting the mutual-exclusion matrix is left to
the per-vendor reference manual.

### Decision 6: `InterruptKind` enum is the cross-vendor superset

Different families publish different IRQ-side fields. The HAL's
`InterruptKind` is the union of every meaningfully-named UART
interrupt across ST / Microchip / NXP:

```cpp
enum class InterruptKind : u8 {
    Tc,           // transfer complete
    Txe,          // tx register empty
    Rxne,         // rx register not empty
    IdleLine,     // bus idle detected
    LinBreak,     // LIN break frame detected
    Cts,          // CTS pin transition
    Error,        // ORE / FE / NE / PE OR-aggregated
    RxFifoThreshold,
    TxFifoThreshold,
};
```

`enable_interrupt(K)` is gated per-K on the corresponding control-
side field (`kRxneIeField`, `kTxeIeField`, etc.). Backends without
the field return `NotSupported`.

### Decision 7: `irq_numbers()` returns the descriptor array

The runtime emits `kIrqNumbers` per peripheral (currently 1 entry
on most STM32 — combined IRQ — and 4+ entries on SAME70 — separate
TX/RX/error/event IRQ lines). The HAL exposes
`port_handle<C>::irq_numbers() -> std::span<const u32>` so users
install ISRs without hardcoding the NVIC line.

The `complete-async-hal` ISR-wiring follow-up
(`add-async-hal-vendor-isr-hooks`) will use this surface to wire
the per-peripheral signal hook generically.

## Risks

- **`set_baudrate` rejection on 16-bit BRR overflow.** Some
  combinations (low kernel clock + low baud rate) overflow the
  16-bit BRR. The HAL detects and returns `InvalidArgument`;
  documented.
- **`KernelClockSource` not reachable via every clock tree.** The
  HAL only checks the published `kKernelClockSourceOptions` list;
  whether the requested source is currently *enabled* in the
  RCC tree is the application's job. The HAL doesn't toggle clocks.
- **FIFO threshold encoding differences.** Some families pack the
  5-value enum into 3 bits; others use 1-bit `enable_or_not`.
  Mitigated by the per-fraction `if constexpr` guard against
  `kFifoTriggerFractionsQ8`.

## Migration

No source-code changes for existing users. The new methods are
additive. Existing examples continue to build and pass on the
3-board matrix.

For users currently writing raw register hex to control FIFO
thresholds or LIN: `docs/UART.md` includes a "before / after"
migration table mapping the hex pattern to the HAL call.
