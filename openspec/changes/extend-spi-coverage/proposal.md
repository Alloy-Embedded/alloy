# Extend SPI Coverage To Match Published Descriptor Surface

## Why

The latest alloy-devices publication ships a rich
`SpiSemanticTraits<P>` covering every CR1 / CR2 / SR / DR field
plus Tier 2/3/4 capability flags
(`kSupportsBidirectional3Wire`, `kSupportsCrc`, `kSupportsDma`,
`kSupportsI2sSubmode`, `kSupportsLsbFirst`, `kSupportsMotorolaFrame`,
`kSupportsNssHwManagement`, `kSupportsTiFrame`,
`kHasIomuxFastPath`) and SAM-style timing fields (`kPcsField`,
`kPcsdecField`, `kDlybcsField`, `kDlybctField`, `kDlybsField`,
`kScbrField`).

The runtime currently consumes ~25% of that surface: the HAL
exposes `configure(SpiConfig{mode, clock_speed, bit_order,
data_size, peripheral_clock_hz}) / transfer / transmit / receive /
is_busy / configure_tx_dma / configure_rx_dma`. `SpiDataSize` is
locked to 8 or 16 bits even though every modern STM32 SPI supports
the full 4–16 range. CRC, TI / Motorola frame format, NSS hardware
management with timing, 3-wire bidirectional mode, kernel-clock
source selection, IRQ-vector lookup, per-CS delay timing — none of
these are accessible from the HAL today.

modm sets the bar for what "complete SPI" looks like in C++ on
embedded: variable data size 4–16 bits with type-system
enforcement, frame-format selection, CRC helpers, hardware NSS with
per-transfer pulsing, and per-vendor SAM-style chip-select timing.
Alloy already publishes the data needed; this change is plumbing.

Concrete scope: lift every published `SpiSemanticTraits<P>` field
and Tier 2/3/4 capability flag into typed HAL methods,
capability-gated via `if constexpr` against
`semantic_traits::k<…>.valid` / `kSupports<…>` flags. Wire the
DMA path into the existing `async::spi::write_dma / read_dma /
transfer_dma` from `complete-async-hal`. Ship a
`spi_probe_complete` example exercising every lever. Document
in `docs/SPI.md`.

## What Changes

### `src/hal/spi/spi.hpp` — extended HAL surface (additive only)

New methods on `port_handle<Connector>`. Every method
capability-gated via `if constexpr` against the published trait
field's `.valid` / `kSupports<…>` flag. Unsupported levers return
`core::ErrorCode::NotSupported`.

- **Variable data size (4–16 bits)**
  - `set_data_size(std::uint8_t bits)` — accepts any value the
    descriptor's `kDsField`/`kDffField` width permits. Returns
    `InvalidArgument` outside `[4, 16]` or when the requested width
    isn't realisable on the peripheral.
  - The closed `SpiDataSize::Bits8` / `Bits16` enum stays as a
    convenience preset; the new method augments it.
- **Frame format**
  - `enum class FrameFormat { Motorola, TI }`.
  - `set_frame_format(FrameFormat)` — gated independently on
    `kSupportsMotorolaFrame` / `kSupportsTiFrame`.
- **CRC (gated on `kSupportsCrc`)**
  - `enable_crc(bool)`, `set_crc_polynomial(std::uint16_t)`,
    `read_crc() -> std::uint16_t`, `crc_error() -> bool`,
    `clear_crc_error()`.
- **Bidirectional 3-wire mode (gated on `kSupportsBidirectional3Wire`)**
  - `set_bidirectional(bool)`, `set_bidirectional_direction(BiDir)`
    where `BiDir` is `Receive` / `Transmit`.
- **Hardware NSS management (gated on `kSupportsNssHwManagement`)**
  - `set_nss_management(NssManagement)` where `NssManagement` is
    `Software` (existing default), `HardwareInput`, `HardwareOutput`.
  - `set_nss_pulse_per_transfer(bool)` — gated on the NSSP field
    where present (STM32 NSSP).
- **Per-CS chip-select timing (SAM-style, gated on `kPcsdecField.valid`)**
  - `set_cs_decode_mode(bool)` — toggles whether PCS bits are
    decoded as 1-of-N (decoded) or 4-of-15 (direct).
  - `set_cs_delay_between_consecutive(std::uint16_t cycles)`,
    `set_cs_delay_clock_to_active(std::uint16_t)`,
    `set_cs_delay_active_to_clock(std::uint16_t)` — wired through
    `kDlybctField` / `kDlybsField` / `kDlybcsField`.
- **Variable baud rate (raw clock_speed, validated)**
  - `set_clock_speed(std::uint32_t hz)` — resolves the divider from
    the peripheral kernel clock; returns `InvalidArgument` if the
    requested rate cannot be realised within ±5 % using the
    descriptor's BR field encoding (powers-of-2 prescaler on STM32,
    full integer prescaler on SAM).
- **Kernel clock source (gated on `kKernelClockSelectorField.valid`)**
  - `set_kernel_clock_source(KernelClockSource)` — same shape as
    the UART variant; runtime-validates against the descriptor's
    published option list.
- **Status flag accessors (typed, all gated)**
  - `tx_register_empty() -> bool`, `rx_register_not_empty() -> bool`,
    `busy() -> bool`, `mode_fault() -> bool`,
    `frame_format_error() -> bool`, `clear_*` mirrors where the
    descriptor publishes a clear-side field.
- **Interrupt enable / disable (typed)**
  - `enum class InterruptKind { Txe, Rxne, Error, ModeFault,
    CrcError, FrameError }`.
  - `enable_interrupt(InterruptKind)` /
    `disable_interrupt(InterruptKind)` — each kind gated on the
    corresponding control-side IE field.
- **NVIC vector lookup**
  - `static constexpr auto irq_numbers() ->
    std::span<const std::uint32_t>` mirrors the UART surface
    landing in `extend-uart-coverage`.

### Per-peripheral typed `Cs` handle

For SAM-family backends with `kPcsField.valid`, codegen will emit
a typed `SpiChipSelectOf<P>::type` enum over the four hardware NPCS
lines (NPCS0–NPCS3). The HAL accepts the typed enum on
`select(Cs)` / `deselect(Cs)`. Until the codegen change lands the
HAL accepts a raw `std::uint8_t` (filed as the codegen follow-up
`add-spi-cs-typed-enum`).

### Async integration

The DMA wrappers from `complete-async-hal`
(`async::spi::write_dma / read_dma / transfer_dma`) compose
unchanged with the new HAL surface. `async::spi::wait_for(InterruptKind)`
is added as a sibling for poll-free wait on `Rxne`, `ModeFault`,
or `CrcError`.

### `examples/spi_probe_complete/`

Targets `nucleo_g071rb`. Configures SPI1 with:
- 16-bit data size, 16 MHz clock, Mode 3, MSB-first
- CRC enabled with polynomial 0x1021 (CCITT)
- Hardware NSS output with NSSP per-transfer pulsing
- DMA TX + RX wired through `async::spi::transfer_dma`

Mirrors a modm `Spi` recipe with each lever.

### Docs

`docs/SPI.md` — comprehensive guide. Sections: model, variable
data-size recipe, CRC recipe, frame-format selection, NSS hardware
management recipe, per-CS timing on SAM, kernel-clock source,
async wiring, modm migration table.

## What Does NOT Change

- Existing `SpiConfig` / `configure` / `transfer` / `transmit` /
  `receive` / DMA APIs are unchanged. New methods are additive on
  `port_handle<C>`.
- SPI tier in `docs/SUPPORT_MATRIX.md` stays `representative` —
  hardware spot-checks for the new levers land per board with the
  existing SAME70 spot-check pattern.
- Descriptor-side fields are NOT changed in this proposal — they're
  already published and stable.

## Out of Scope (Follow-Up Changes)

- **`add-spi-cs-typed-enum` (alloy-codegen).** Mirrors
  `add-adc-channel-typed-enum`: emits `SpiChipSelectOf<P>::type` per
  peripheral so the HAL drops its raw `std::uint8_t` shim and
  consumes typed CS lines. Tracked separately because it pairs with
  the codegen indexed-field machinery.
- **I²S sub-mode (`kSupportsI2sSubmode`).** Some STM32 SPI peripherals
  re-purpose into I²S audio. The descriptor publishes the flag but
  not the supporting register set. Tracked as
  `add-i2s-coverage` follow-up once the descriptor publishes the
  audio-side surface.
- **DMA double-buffer / circular RX.** Same posture as the UART
  variant: the runtime supports it through
  `enable_dma(circular=true)`; a typed "ring-buffer-and-flag-when-half"
  recipe ships as `spi_circular_dma` example.
- **ESP32 / RP2040 / AVR-DA SPI coverage.** The descriptors with
  `kPresent=true` SPI get the same surface; the in-flight kernel-
  clock-traits + irq-vector-traits work in alloy-codegen needs to
  reach those families before this change applies meaningfully to
  them.
- **Hardware spot-checks for the new levers.** Land per board with
  the existing `spi_probe` matrix in a follow-up
  `validate-spi-coverage-on-3-boards`.

## Alternatives Considered

**Generate the entire SPI HAL from the schema id.** Same posture
as ADC and UART: schema-aware code violates the runtime/device
boundary. Capability-flag + typed-field-ref absorbs every
difference between ST's CR1/CR2 layout and SAM's MR/CSR layout
without naming schemas.

**Expose `SpiDataSize` as a wider closed enum (Bits4..Bits16).**
Rejected — modm uses `Bits<N>` template syntax which is heavier;
raw `std::uint8_t bits` plus runtime validation against the
descriptor's published width is the simpler shape and matches
what `set_baudrate(u32)` does for UART.

**Wait for ESP32 / RP2040 / AVR-DA SPI traits to fill out before
extending.** Rejected — same reasoning as UART. The 3-board
ST/Microchip/NXP matrix is foundational and shouldn't wait on
every family.
