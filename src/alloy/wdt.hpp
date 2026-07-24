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

// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the watchdog IP (a
// board without one never includes a vendor driver it can't compile).
#include "alloy/hal/watchdog/wdt_impl.hpp"

namespace alloy::wdt {

template <class Inst>
class watchdog {
public:
    void start(std::chrono::milliseconds timeout) const {
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
