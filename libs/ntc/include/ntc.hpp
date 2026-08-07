// NTC thermistor conversion — pure math, no hardware: ADC counts in,
// resistance and temperature out. The circuit model is the classic ladder
// (pull-up R_p from Vref to the sense node, NTC from the node to ground),
// so R_ntc = R_p * counts / (full_scale - counts); rails mean a broken
// sensor and are reported as such, never as a temperature.
//
// Temperature uses the beta equation 1/T = 1/T0 + ln(R/R0)/beta, with beta
// optionally LINEAR IN ln(R): beta(R) = b0 + b1*ln(R). b1 = 0 is the plain
// fixed-beta everyone publishes; the ln-linear form fits real curves far
// better across wide ranges and costs one multiply. Float on purpose — this
// is the generic lib; a fixed-point port belongs to the application that
// needs it.
#pragma once

#include <cmath>
#include <cstdint>

#include "alloy/util/result.hpp"

namespace alloy::lib::ntc {

enum class fault : std::uint8_t {
    open = 1,   // counts pinned at the top rail: sensor disconnected
    shorted,    // counts pinned at ground: sensor/harness short
};

struct config {
    float pullup_ohm = 10'000.0f;
    float r0_ohm = 10'000.0f;     // resistance at t0
    float t0_celsius = 25.0f;
    float beta0 = 3'435.0f;       // b0
    float beta_ln = 0.0f;         // b1: beta(R) = b0 + b1*ln(R); 0 = fixed beta
    std::uint16_t full_scale = 0xFFFFu;
    std::uint16_t rail_margin = 64;  // counts within a rail = broken sensor
};

class converter {
public:
    constexpr explicit converter(config c) : c_(c) {}

    [[nodiscard]] Result<float, fault> resistance(std::uint16_t counts) const {
        if (counts >= c_.full_scale - c_.rail_margin) {
            return fault::open;
        }
        if (counts <= c_.rail_margin) {
            return fault::shorted;
        }
        const float n = static_cast<float>(counts);
        return c_.pullup_ohm * n / (static_cast<float>(c_.full_scale) - n);
    }

    [[nodiscard]] Result<float, fault> celsius(std::uint16_t counts) const {
        const auto r = resistance(counts);
        if (!r) {
            return r.error();
        }
        const float ln_ratio = std::log(*r / c_.r0_ohm);
        const float beta = c_.beta0 + c_.beta_ln * std::log(*r);
        const float t0_k = c_.t0_celsius + 273.15f;
        const float inv_t = 1.0f / t0_k + ln_ratio / beta;
        return 1.0f / inv_t - 273.15f;
    }

private:
    config c_;
};

}  // namespace alloy::lib::ntc
