// User-facing DAC: a stateless handle over the generated DAC instance,
// satisfying the DacChannel concept.
//
//   board::dac.enable();
//   board::dac.write(2048);   // ~half of VREF on the DAC output pin
//
// The output pin (DAC_OUT1) is fixed by silicon and resets to analog mode, so
// there is nothing to bind — enable + write is the whole contract.

#pragma once

#include <cstdint>

// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the DAC IP.
#include "alloy/hal/dac/dac_impl.hpp"

namespace alloy::dac {

template <class Inst>
class channel {
public:
    void enable() const { hal::dac_impl<Inst>::enable(); }
    // Raw right-aligned code (0 .. full-scale) -> voltage on the output pin.
    void write(std::uint16_t code) const { hal::dac_impl<Inst>::write(code); }
};

// No-op stand-in for boards without a dac role — keeps board::dac.write()
// compiling everywhere.
struct null_channel {
    void enable() const {}
    void write(std::uint16_t) const {}
};

}  // namespace alloy::dac
