# Design: Extend SPI Coverage

## Context

The runtime exposes the bare-minimum SPI HAL (`configure / transfer
/ transmit / receive / DMA wrappers`). The descriptor publishes
50+ field refs plus eight Tier-2/3/4 capability flags
(`kSupportsCrc`, `kSupportsTiFrame`, `kSupportsNssHwManagement`,
`kSupportsBidirectional3Wire`, etc.). This document captures the
design for closing the gap without breaking the SAME70 hardware
spot-check matrix and without crossing the runtime/device
boundary.

## Goals

1. Match modm-class SPI completeness (variable 4–16 bit data size,
   CRC, frame format, hardware NSS with timing, per-CS delays,
   kernel-clock source) on every device the descriptor declares
   SPI for.
2. Stay schema-agnostic: the HAL reaches for typed
   `RuntimeRegisterRef` / `RuntimeFieldRef` plus the Tier-2/3/4
   capability flags — never for vendor schema names.
3. Compose cleanly with `complete-async-hal`'s
   `async::spi::write_dma / read_dma / transfer_dma`.
4. Keep the existing API byte-compatible — every change is additive.

## Non-Goals

- I²S sub-mode. Descriptor publishes the flag but not the supporting
  register set. Tracked as `add-i2s-coverage` follow-up.
- ESP32 / RP2040 / AVR-DA SPI feature parity. They get the same
  surface as the publication catches up.
- Per-CS typed `SpiChipSelectOf<P>` enum. Lives in alloy-codegen as
  a focused follow-up; the HAL accepts a raw `std::uint8_t`
  transitional shim until then (mirror of how
  `extend-adc-coverage` started).

## Key Decisions

### Decision 1: Capability gates via `if constexpr`

Same pattern as ADC and UART. The HAL uses
`if constexpr (semantic_traits::kSupportsCrc)` /
`if constexpr (semantic_traits::kPcsField.valid)` to either emit
the real implementation or fall through to
`core::ErrorCode::NotSupported`. The compiler eliminates the unused
branch entirely.

### Decision 2: `set_data_size(std::uint8_t bits)` accepts 4–16 with runtime validation

The descriptor's `kDsField` (STM32 G0/F4 SPI: 4 bits = 4–16
encoding) and `kDffField` (older STM32: 1 bit = 8/16) carry
different widths. The HAL accepts a raw `std::uint8_t` and:

1. `static_assert(bits >= 4 && bits <= 16)` if the value is a
   compile-time constant.
2. Returns `core::ErrorCode::InvalidArgument` at runtime if the
   peripheral's published field width can't represent the requested
   value (e.g. requesting `Bits12` on an `kDffField`-only peripheral
   that only supports 8 / 16).

The closed `SpiDataSize::Bits8` / `Bits16` enum stays as a typed
convenience preset — most users want 8.

### Decision 3: `set_clock_speed(u32 hz)` validates against ±5 %

SPI baud divider encodings vary: STM32 uses powers-of-2 (2, 4, 8,
…, 256), SAM uses an integer prescaler (1–255). Both compute a
realised clock that may differ from the requested rate. The HAL:

1. Computes the realised rate from the descriptor's BR field width
   + peripheral kernel clock.
2. Returns `core::ErrorCode::InvalidArgument` if the realised rate
   is outside ±5 % of the request.

±5 % (vs UART's ±2 %) reflects that SPI peripherals are typically
flexible on exact baud — application timing is dominated by
device-side requirements (e.g. "max 10 MHz"), not exact match.

### Decision 4: Typed `NssManagement` enum captures the full SPI NSS state machine

The HAL's `NssManagement` covers:

- `Software` — SSM bit set, SSI controlled by application
- `HardwareInput` — SSM cleared, NSS pin is master input
- `HardwareOutput` — SSOE set, NSS driven low while SPE active

Plus `set_nss_pulse_per_transfer(bool)` which toggles NSSP (where
present). This matches the modm `Nss` API surface exactly.

### Decision 5: Per-CS SAM timing fields exposed as raw cycle counts

The SAM SPI's DLYBCS / DLYBCT / DLYBS fields encode delays in
peripheral-clock cycles. The HAL exposes raw `std::uint16_t cycles`
setters; the application is responsible for translating its
microsecond-side requirement into cycles using
`kernel_clock_hz()`. This matches what the SAM datasheet expects
applications to do; trying to expose microseconds at the HAL level
would either lose precision or require floating-point in the HAL.

### Decision 6: `InterruptKind` enum is the cross-vendor superset

Same pattern as UART. The enum covers `Txe`, `Rxne`, `Error`,
`ModeFault`, `CrcError`, `FrameError`. Each kind gates per-K on
the corresponding control-side IE field; backends without the
field return `NotSupported`.

### Decision 7: `irq_numbers()` accessor mirrors UART

Same shape: `static constexpr` returning the descriptor's
`kIrqNumbers` so `complete-async-hal`'s vendor ISR hooks
(`add-async-hal-vendor-isr-hooks` follow-up) wire generically.

## Risks

- **`set_data_size(12)` on `kDffField`-only peripherals.** Some
  legacy SPI peripherals only support 8/16. The HAL returns
  `InvalidArgument`; documented.
- **`set_clock_speed` rejecting realisable rates due to ±5 %.**
  SPI prescalers leave large gaps (especially STM32 powers-of-2).
  Some "exact" rate requests will fail. Documented; users can
  accept the realised rate by querying
  `realised_clock_speed() -> u32` (a sibling helper that returns
  what the BR encoding produces without writing it).
- **CRC polynomial register width varies.** STM32 SPI CRC is a
  16-bit polynomial; SAM SPI doesn't have CRC at all
  (`kSupportsCrc = false`); LPSPI on iMXRT has its own variant.
  The HAL exposes `set_crc_polynomial(std::uint16_t)`; backends
  with a wider polynomial would need a follow-up extension. None
  in the foundational set today.

## Migration

No source-code changes for existing users. The new methods are
additive. Existing examples continue to build and pass.

For users currently writing raw register hex to control CRC or
hardware NSS: `docs/SPI.md` includes a "before / after" migration
table mapping the hex pattern to the HAL call.
