# Extend DAC Coverage To Match Published Descriptor Surface

## Why

`DacSemanticTraits<P>` publishes typed register / field refs for
control / status / mode / trigger / data registers, plus channel-
addressed enable / disable / ready / data indexed-field patterns,
plus capability flags (`kHasDma`, `kHasHardwareTrigger`),
prescaler, software-reset, trigger-select, and `kIrqNumbers` /
`kKernelClockSelectorField` Tier 2/3/4 metadata.

The runtime currently consumes ~30%: the HAL exposes
`configure / enable / disable / ready / write / configure_dma`.
Hardware trigger source / edge, prescaler, software reset,
per-channel addressing on multi-channel DACs, status flags,
typed interrupts — none reachable today.

modm covers DAC end-to-end. Alloy already publishes the data
needed; this change is plumbing.

## What Changes

### `src/hal/dac.hpp` — extended HAL surface (additive only)

- **Per-channel addressing** (gated on `kChannelEnablePattern.valid`)
  - `enable_channel(std::uint8_t)` / `disable_channel(std::uint8_t)`
    / `channel_ready(std::uint8_t) -> bool`.
  - `write_channel(std::uint8_t, std::uint32_t value)` —
    independent of `write` (which targets channel 0).
- **Hardware trigger** (gated on `kHasHardwareTrigger`)
  - `set_hardware_trigger(std::uint8_t source, TriggerEdge edge)`
    where `TriggerEdge` covers `Disabled`, `Rising`, `Falling`,
    `Both`. The `source` integer is clamped to the descriptor's
    `kTriggerSelectField` width.
- **Prescaler / sample timing**
  - `set_prescaler(std::uint16_t)` — gated on `kPrescalerField.valid`.
- **Software reset**
  - `software_reset() -> Result<void, ErrorCode>` — gated on
    `kSoftwareResetField.valid`.
- **Status flags / interrupts** (typed)
  - `enum class InterruptKind { TransferComplete, Underrun,
    DmaComplete }`.
  - `enable_interrupt(InterruptKind)` /
    `disable_interrupt(InterruptKind)` — per-kind gated.
  - `underrun() -> bool`, `clear_underrun()`.
- **Kernel clock source** (gated on
  `kKernelClockSelectorField.valid`)
  - `set_kernel_clock_source(KernelClockSource)`.
- **NVIC vector lookup**
  - `irq_numbers() -> std::span<const std::uint32_t>`.
- **Async sibling**
  - `async::dac::wait_for(InterruptKind)` extends
    `complete-async-hal`.

### `examples/dac_probe_complete/`

Targets `nucleo_g071rb` DAC1. Configures channel 1 with hardware
trigger from TIM3 update at 1 kHz, async write loop driven by
DMA from a sine-wave sample buffer.

### Docs

`docs/DAC.md` — model, channel addressing, trigger recipes,
underrun handling, async wiring, modm migration table.

## What Does NOT Change

- Existing DAC API unchanged. New methods are additive.
- DAC tier in `docs/SUPPORT_MATRIX.md` stays `representative` —
  hardware spot-checks for new levers land per board.

## Out of Scope (Follow-Up Changes)

- `add-dac-channel-typed-enum` (alloy-codegen) — typed channel id
  per peripheral, mirror of `add-adc-channel-typed-enum`.
- ESP32 / RP2040 / AVR-DA DAC parity — gated on alloy-codegen.
- Hardware spot-checks → `validate-dac-coverage-on-3-boards`.

## Alternatives Considered

Same posture as ADC: capability-flag + typed-field-ref absorbs
vendor differences without naming schemas. `set_hardware_trigger`
takes raw source index until codegen publishes a typed
`DacTriggerSource<P>` enum.
