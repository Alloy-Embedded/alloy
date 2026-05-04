# Tasks: Extend ADC Coverage

Phases are ordered. Phases 1-3 are host-testable (compile + concept).
Phase 4 needs the existing 3-board hardware matrix
(SAME70 / STM32G0 / STM32F4) — same boards that already validate
`adc` at the foundational tier today.

## 1. Channel typed enum (codegen + runtime)

- [x] 1.1 alloy-codegen: extend the ADC semantic emitter so each
      published peripheral also produces a typed
      `enum class Channel : std::uint8_t { … }` listing the
      channels (peripheral + internal) declared in the descriptor.
      Channels are named from the SVD where available
      (`CH0`, `CH1`, …, `Vrefint`, `VBat`, `TempSensor`); fallback is
      ordinal-only.
      (implemented in alloy-codegen as `add-adc-channel-typed-enum`)
- [x] 1.2 alloy-codegen: regenerate goldens for ST / Microchip / NXP
      so the new `Channel` enum appears in
      `<vendor>/<family>/.../driver_semantics/adc.hpp` next to
      `AdcSemanticTraits`.
      (`AdcChannelOf<P>` + `AdcChannel<P>` alias already in alloy-devices)
- [x] 1.3 Runtime: `src/hal/adc.hpp` exposes `using Channel =
      device::AdcChannel<Peripheral>` (the codegen-emitted enum).
      Existing call sites that pass `std::uint8_t` keep compiling
      via implicit conversion; new call sites are typed.
      Also wired `AdcChannelOf`/`AdcChannel` into `src/device/runtime.hpp`;
      fixed `kAlignmentField` → `kAlignField` bug in `set_alignment`.

## 2. HAL extensions

All methods are members of `alloy::hal::adc::handle<Peripheral>`.
Every method `static_assert(valid)` first; gates the body via
`if constexpr` on the published trait field's `.valid`. Returns
`core::ErrorCode::NotSupported` on the fall-through path.

- [x] 2.1 Resolution / alignment:
      - `enum class Resolution { Bits6, Bits8, Bits10, Bits12, Bits14, Bits16 }`
      - `enum class Alignment { Right, Left }`
      - `set_resolution(Resolution)` — clamped at compile time to
        `kResultBits` ceiling (`static_assert` if the requested value
        exceeds the published max).
      - `set_alignment(Alignment)`.
- [x] 2.2 Mode:
      - `set_continuous(bool)`
      - `stop()` (inverse of existing `start()`).
- [x] 2.3 Sample time:
      - `set_sample_time(Channel, std::uint32_t ticks)` — only
        emits MMIO when `kSampleTimeRegister.valid`.
- [x] 2.4 Sequence builder:
      - `set_sequence(std::span<const Channel> channels)` — programmes
        ordered conversion sequence using `kSequenceRegister` +
        `kChannelBitPattern`. Empty span clears the sequence; oversize
        returns `core::ErrorCode::InvalidArgument`.
- [x] 2.5 Per-channel enable (Microchip-style):
      - `enable_channel(Channel)` / `disable_channel(Channel)` /
        `channel_enabled(Channel) -> bool` — gated on
        `kChannelEnablePattern.valid`.
- [x] 2.6 Hardware trigger:
      - `enum class TriggerEdge { Disabled, Rising, Falling, Both }`
      - `set_hardware_trigger(std::uint8_t source, TriggerEdge edge)` —
        v1 takes raw source index per the descriptor's
        `kExternalTriggerSelectField` width (clamped); a future
        codegen change can promote to a typed `TriggerSource` enum.
- [x] 2.7 Sequence read (no-DMA path):
      - `read_sequence(std::span<std::uint16_t> samples)` — drains
        `samples.size()` conversions by polling
        `kEndOfConversionField` and reading `kDataField` in a loop.
        Returns `core::ErrorCode::AdcOverrun` if `kOverrunField` flips
        mid-loop.
- [x] 2.8 Status:
      - `end_of_sequence() -> bool` (`kEndOfSequenceField`)
      - `overrun() -> bool` (`kOverrunField`)
      - `clear_overrun() -> Result<void, ErrorCode>` (clears the
        field).

## 3. Compile tests

- [x] 3.1 Extend `tests/compile_tests/test_adc_api.cpp`: instantiate
      every new method against the existing `nucleo_g071rb` device
      contract. Verify return types, `static_assert` fires on
      out-of-range `Resolution`.
- [x] 3.2 Add a SAME70-targeted compile test exercising
      `enable_channel` / `disable_channel` (the AFEC per-channel
      register path that doesn't exist on G0 / F4).
- [x] 3.3 Concept test: `static_assert(handle<…>::has_resolution() ==
      true)` etc. for each capability — codified introspection so
      examples can branch without consulting docs.

## 4. Async integration

- [x] 4.1 Extend `src/runtime/async_adc.hpp`'s `scan_dma`: take an
      additional `complete_on` parameter
      (`CompletionTrigger::DmaTransferComplete` or
      `CompletionTrigger::EndOfSequence`); the runtime hook uses the
      requested signal source. Default keeps backward compat
      (DMA TC).
- [x] 4.2 Extend `tests/compile_tests/test_async_peripherals.cpp`
      with a `scan_dma(complete_on=EndOfSequence)` instantiation.

## 5. Example

- [x] 5.1 `examples/analog_probe_complete/`: targets
      `nucleo_g071rb`. Configures ADC1 with:
      - resolution 12-bit, right alignment
      - 4-channel sequence (CH0, CH1, CH4, Vrefint) via typed Channel enum
      - per-channel sample time (slow on Vrefint, fast on the rest)
      - hardware trigger configured (TIM3_TRGO source index 3) then disabled
        so the read_sequence section runs on software trigger
      - continuous mode + polling read_sequence (DMA circular deferred to
        when ADC DMA bindings land in alloy-devices)
      - overrun monitor loop: re-enable continuous, print "OVERRUN #N" on
        each overrun event detected via adc.overrun() + clear_overrun()
      Demonstrates every new lever in one focused demo. Builds for
      `nucleo_g071rb`, `same70_xplained`, and `nucleo_f401re` from a single
      main.cpp with conditional blocks.
- [x] 5.2 Mirror configuration on `same70_xplained` targeting AFEC0
      (uses `enable_channel` / `disable_channel` instead of set_sequence).
- [x] 5.3 Mirror on `nucleo_f401re` for ADC1 (third foundational board).

## 6. Hardware spot-check (3-board matrix)

- [ ] 6.1 SAME70 Xplained: run `analog_probe_complete`. Verify all
      4 channels report sane voltages; trigger jitter < 1% at 1 kHz;
      no overrun across 60 s.
- [ ] 6.2 STM32G0 Nucleo: same matrix.
- [ ] 6.3 STM32F4 Nucleo: same matrix.
- [x] 6.4 Update `docs/SUPPORT_MATRIX.md` `adc` row to record the
      extended-coverage validation (single line note + link to
      `docs/ADC.md`).

## 7. Documentation

- [x] 7.1 `docs/ADC.md` — comprehensive guide:
      Created with: model, single-shot + SAME70 quick-start, resolution,
      alignment, continuous, sample time, sequence, per-channel enable,
      hardware trigger, overrun, DMA, async, capability matrix, related links.
- [x] 7.2 Reference `docs/ADC.md` from `docs/ASYNC.md` (under the
      ADC section) and `docs/COOKBOOK.md` (where applicable).
- [x] 7.3 Cross-link from `docs/SUPPORT_MATRIX.md` `adc` row.
      Done — ADC row updated + "See [ADC.md](ADC.md)" added.
      ADC section in ASYNC.md now links to ADC.md.

## 8. Out-of-scope follow-ups (filed but not done in this change)

- [ ] 8.0 **alloy**: `add-peripheral-clock-gate-to-configure` openspec filed
      at `openspec/changes/add-peripheral-clock-gate-to-configure/`. Until
      implemented, `read_sequence` returns `AdcConversionTimeout` on SAME70
      (AFEC clock not enabled) and STM32G0/F4 (ADC clock not enabled via RCC).
      All other levers work correctly on hardware.
- [ ] 8.0b **alloy-codegen**: file `add-adc-sample-time-pattern` — STM32G0
      ADC1 has per-channel sample time (SMPR register, 3 bits per channel) but
      codegen does not publish `kSampleTimePattern` (indexed field ref).
      Without it `has_sample_time()` returns false and `set_sample_time()`
      returns `NotSupported`. Validated on STM32G071RB and SAME70 Xplained.
- [ ] 8.1 File `add-adc-coverage-esp32` once Espressif ADC schema is
      published in alloy-devices.
- [ ] 8.2 File `add-adc-coverage-rp2040` once RP2040 ADC schema is
      published.
- [ ] 8.3 File `add-adc-coverage-avr-da` once AVR-DA ADC schema is
      published.
- [ ] 8.4 File `add-adc-injected-channels` once descriptor publishes
      injected-mode fields.
- [ ] 8.5 File `add-adc-watchdog` once descriptor publishes
      analog-window fields.
- [ ] 8.6 File `add-adc-calibration` once descriptor publishes
      calibration coefficient fields.
- [ ] 8.7 File `add-adc-differential` once descriptor publishes
      differential-channel fields.
