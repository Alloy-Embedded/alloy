// User-facing watchdog: a stateless, zero-size handle over the generated WDT
// instance, satisfying the Watchdog concept.
//
//   board::watchdog.start(4s);   // reset the MCU if not fed within 4 s
//   for (;;) { work(); board::watchdog.feed(); }
//
// The timeout is programmed once (start()); feed() is the periodic kick. On
// parts whose watchdog is write-once and enabled at reset (SAM E70), declaring
// the board's watchdog role makes codegen leave the counter running for this
// start() to configure — see the driver header.

#pragma once

#include <chrono>
#include <cstdint>

#include "alloy/core/claim.hpp"
// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the watchdog IP (a
// board without one never includes a vendor driver it can't compile).
#include "alloy/hal/watchdog/wdt_impl.hpp"

namespace alloy::wdt {

template <class Inst>
class watchdog {
public:
    // SHARED with the timeout as the witness, and this facade is where the
    // block-config hole was widest. `start()` programs one prescaler and one
    // reload for the whole peripheral, there is no close(), and nothing
    // recorded that it had been called — so a `start(4s)` in main() and a
    // `start(10s)` in a service both "succeeded", the second silently replaced
    // the first, and the program was left believing in a deadline the silicon
    // had stopped enforcing. That is hole (A) with a value attached, exactly
    // the PWM frequency case on a different block.
    //
    // Shared rather than exclusive because arming the same timeout twice is
    // not a contradiction (a bootloader and the app it starts may both mean
    // "2 s"), and there is no third claimant to exclude: `feed()` claims
    // nothing, so the hot path is untouched.
    void start(std::chrono::milliseconds timeout) const {
        alloy::claim::shared<Inst, alloy::claim::personality::wdt>(
            static_cast<std::uint32_t>(timeout.count()));
        hal::wdt_impl<Inst>::start(timeout);
    }
    void feed() const { hal::wdt_impl<Inst>::feed(); }
};

// No-op stand-in for boards without a watchdog role — keeps
// `board::watchdog.feed()` compiling everywhere (guard #6).
struct null_watchdog {
    void start(std::chrono::milliseconds) const {}
    void feed() const {}
};

}  // namespace alloy::wdt
