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

}  // namespace alloy::hal
