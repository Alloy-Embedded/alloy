// exti_impl<Inst> — primary template, intentionally undefined.
//
// The "EXTI" class of block is the one that turns a GPIO PIN EDGE into a CPU
// interrupt. One partial specialization exists per IP version (st_exti_g0.hpp,
// ...), constrained on the instance's IP tag type. Using a controller whose IP
// has no driver is a compile error pointing here.
//
// This header is included UNCONDITIONALLY by the GPIO drivers, so a pin driver
// can name exti_impl<...> in a `requires`-clause on chips that have no curated
// EXTI at all — there the port has no `exti` companion, the requires-clause is
// false, and the pin-interrupt methods simply do not exist.

#pragma once

#include <cstdint>

#include "alloy/core/claim.hpp"

namespace alloy::hal {

// Which transition arms an interrupt. A RUNTIME value, not a template
// parameter: it maps 1:1 onto the trigger-enable bits, and re-arming one pin
// with a different edge must not instantiate a second handler and a second
// callback slot.
enum class pin_edge : std::uint8_t { rising, falling, both };

// ── WHO OWNS AN EXTI LINE ────────────────────────────────────────────────
//
// A LINE, not a controller, is the contested resource here, and the claimants
// are PINS. On every ST part one line serves the same index on every port —
// PA5, PB5, PC5 and PD5 are all EXTI line 5 — and one EXTICR field, one
// trigger pair and one callback slot serve whichever of them arrived last.
// Arming two of them used to compile clean and say nothing: the second pin
// took the line, the first pin's handle stayed "armed", its callback never ran
// again, and `edges()` went on answering — with the OTHER pin's edge count,
// because the counter is keyed on the line. That is hole (A) of
// docs/reference/peripheral-surface.md, one scope down from the peripheral.
//
// These two helpers are the contract every exti driver honours, stated once
// here rather than per IP: `arm()` claims, `disarm()` releases, and the port
// index is the witness that distinguishes re-arming the SAME pin (legal, and
// documented — it replaces the edge and the callback) from a SECOND pin
// stealing the line (a bug, and now a typed trap).
//
// Deliberately NOT interrupt-masked. Every other alloy::claim call site is the
// same, with dma::channel::claim() the one exception, and arm() is already a
// sequence of unguarded read-modify-writes on IMR1/RTSR1/FTSR1 — masking one
// byte of it would buy nothing the rest of the function does not give away.
template <class Inst, unsigned Line>
inline void claim_exti_line(unsigned port_index) {
    alloy::claim::sub_shared<Inst, Line, alloy::claim::personality::exti>(port_index);
}

template <class Inst, unsigned Line>
inline void release_exti_line(unsigned port_index) {
    alloy::claim::sub_release<Inst, Line, alloy::claim::personality::exti>(port_index);
}

template <class Inst>
struct exti_impl;

}  // namespace alloy::hal
