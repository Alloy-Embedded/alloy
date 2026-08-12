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

#include "alloy/core/claim.hpp"
// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the DAC IP.
#include "alloy/hal/dac/dac_impl.hpp"

namespace alloy::dac {

template <class Inst>
class channel {
public:
    // SHARED WITH A CONSTANT WITNESS, and the honest reason is worth stating:
    // this facade carries NO configuration, so the "contradictory config, last
    // writer wins" half of hole (A) cannot happen here — two
    // `dac::channel<dac1_t>` objects are indistinguishable and `enable()` is
    // idempotent. What the claim buys is the OTHER half: the block is now on
    // record as running its DAC personality, so the day a facade claims the
    // same instance as something else, it traps instead of quietly coexisting.
    // A constant witness is what makes `enable()` safe to call twice; an
    // exclusive claim here would invent a bug that does not exist.
    void enable() const {
        alloy::claim::shared<Inst, alloy::claim::personality::dac>(0u);
        hal::dac_impl<Inst>::enable();
    }
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
