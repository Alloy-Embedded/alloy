// adc_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per ADC IP version, constrained on the
// instance's IP tag type.
//
// This header also carries the SHARED VOCABULARY — the one file both the
// facade and every driver include — so the words are common without the
// facade depending on any driver, exactly as uart_impl.hpp does for the UART
// (docs/reference/peripheral-surface.md).

#pragma once

#include <cstdint>

namespace alloy::hal {

template <class Inst>
struct adc_impl;

// ── Layer 1 vocabulary for ONE analog watchdog ──────────────────────────
//
// Re-exported as `alloy::adc::watchdog_config`; see alloy/adc.hpp for what the
// units mean and for the three things the silicon forced into this shape.
//
// One CHANNEL and not a channel set, and that is the admission test doing its
// job rather than an oversight: ST's adc_v2 has three watchdogs and only two
// of them can guard an arbitrary set — watchdog 1 guards one channel or all of
// them. A `channels` bitmask here would be a portable field that works at two
// of three ordinals on the very first IP that has any.
struct adc_watchdog_config {
    std::uint8_t channel = 0;
    std::uint16_t low = 0;
    std::uint16_t high = 0;
};

// ── Layer 2: the per-IP knob bag ────────────────────────────────────────
//
// Primary template: EMPTY, and always usable. An ADC driver with no vendor
// knobs curated inherits this and its `configure<O>` ignores O, so adding
// Layer 2 costs zero lines in a driver that has nothing to add — the same
// shape uart_opts uses (docs/reference/peripheral-surface.md, "Layer 2").
//
// Re-exported as `alloy::adc::opts<Inst>`; a user never types `hal`.
//
// THE NAMING RULE APPLIES HERE AND IS THE WHOLE POINT: the same silicon
// feature gets the same field name and the same unit in every driver that has
// it, so portable code probes by name with no preprocessor —
//
//     if constexpr (requires { Opts{}.oversample_ratio; }) { … }
//
// The vocabulary this ADC surface registers, with its units, so the next
// driver does not invent a synonym:
//
//   resolution_bits          bits in the result the converter produces
//   oversample_ratio         how many samples are summed per result; 1 = off
//   oversample_shift         right shift applied to that sum
//   sample_time              how long the input is tracked, as an IP enum
//   sample_time_alt          the SECOND shared tracking time, where the IP has
//                            exactly two rather than one per channel
//   sample_time_alt_channels bitmask of channels that use sample_time_alt
//   trigger / trigger_edge   the hardware event that starts a conversion
//   prescaler                divider from the ADC clock source to the converter
//
// A unit that is a register artefact stays a Layer-2 name forever, which is
// why sample_time is an enum of the IP's own durations rather than
// nanoseconds: the conversion to time needs the ADC kernel clock, and a field
// that silently means something else per chip is the lie this layer removes.
template <class Inst>
struct adc_opts {};

}  // namespace alloy::hal
