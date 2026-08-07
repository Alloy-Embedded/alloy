// What the firmware knows about the crash that came BEFORE this boot.
//
// A product that hangs in the field is a support call with no evidence. The
// generated vector table points every fault at a weak Default_Handler, and the
// default body is `for (;;) {}` — a dead device that says nothing. This is the
// other half: the fault handler records where it died into RAM that survives a
// reset, resets, and the next boot reads the record out of `take()`.
//
// The record is deliberately small and flat: it is written by a handler running
// on a corrupted machine, so it touches nothing that could itself fault — no
// allocation, no driver, no clock reprogramming, no formatting.
//
// Usage is one call, right after board::init():
//
//     if (alloy::fault::record crash; alloy::fault::take(crash)) {
//         uart.write("recovered from a crash at pc=");
//         // ... print crash.pc / crash.status; `alloy crash` decodes it
//     }
//
// `take()` clears the record, so a crash is reported once. `consecutive()` does
// NOT clear: it counts boots that ended in a fault, which is how firmware
// notices it is in a crash loop and stops doing whatever causes it.

#pragma once

#include <cstdint>

namespace alloy::fault {

enum class cause : std::uint8_t {
    none = 0,
    hard,   // a fault, or a configurable fault that escalated into one
    nmi,
};

struct record {
    cause what = cause::none;
    // How many boots in a row ended in a fault. 1 means this was the first.
    std::uint8_t consecutive = 0;
    // The stacked exception frame: where the core was when it died.
    std::uint32_t pc = 0;
    std::uint32_t lr = 0;
    std::uint32_t psr = 0;
    std::uint32_t sp = 0;   // stack pointer BEFORE the core stacked the frame
    // ARMv7-M: the Configurable Fault Status Register, which says WHICH fault.
    // ARMv6-M (Cortex-M0/M0+) has no fault status registers at all and leaves
    // this 0 — the honest answer, not a fabricated one.
    std::uint32_t status = 0;
    // The faulting address, when `status` says one was recorded. 0 otherwise.
    std::uint32_t address = 0;
};

/// Take the crash record left by the previous boot, if there is one. Returns
/// false when this boot follows a clean start (power-on, or a normal reset).
/// Clears the record: a crash is reported once.
[[nodiscard]] bool take(record& out) noexcept;

/// Boots in a row that ended in a fault, surviving `take()`. Use it to refuse
/// to re-enter whatever keeps crashing: `if (consecutive() >= 3) { safe_mode(); }`
[[nodiscard]] std::uint8_t consecutive() noexcept;

/// Declare this boot healthy — clears the consecutive counter. Same shape as an
/// OTA trial confirming itself, and for the same reason: only the application
/// knows it got far enough to count as working, so nothing can clear the
/// counter on its behalf. Without this call the count only ever grows.
void healthy() noexcept;

}  // namespace alloy::fault
