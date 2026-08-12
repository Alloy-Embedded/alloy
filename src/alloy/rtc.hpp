// User-facing real-time clock: a stateless handle over the generated RTC
// instance, satisfying the Rtc concept.
//
//   board::rtc.set({.hour = 12, .minute = 0, .second = 0});
//   auto t = board::rtc.now();   // keeps ticking, survives reset (backup domain)
//
// The RTC clock source (LSI) is brought up by the board clock program when the
// board declares an `rtc` role; this handle only sets/reads the calendar.

#pragma once

#include "alloy/concepts.hpp"
#include "alloy/core/claim.hpp"
// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the RTC IP.
#include "alloy/hal/rtc/rtc_impl.hpp"

namespace alloy::rtc {

template <class Inst>
class rtc {
public:
    // `set()` is where this facade touches the block's init sequence, so it is
    // where the claim goes — shared with a constant witness, because a wall
    // clock is DATA and setting it twice (first boot, then an NTP correction)
    // is the normal thing to do, not a contradiction. As with dac/can, the
    // claim records the personality; it does not police the value. `now()`
    // only reads, and claims nothing.
    void set(const datetime& dt) const {
        alloy::claim::shared<Inst, alloy::claim::personality::rtc>(0u);
        hal::rtc_impl<Inst>::set(dt);
    }
    [[nodiscard]] datetime now() const { return hal::rtc_impl<Inst>::now(); }
    // True once the calendar has been set — lets an app set the clock on first
    // boot only and just read it after (it keeps ticking across resets).
    [[nodiscard]] bool initialized() const { return hal::rtc_impl<Inst>::initialized(); }
};

// No-op stand-in for boards without an rtc role — keeps board::rtc.now()
// compiling everywhere (returns a zero datetime).
struct null_rtc {
    void set(const datetime&) const {}
    [[nodiscard]] datetime now() const { return {}; }
    [[nodiscard]] bool initialized() const { return false; }
};

}  // namespace alloy::rtc
