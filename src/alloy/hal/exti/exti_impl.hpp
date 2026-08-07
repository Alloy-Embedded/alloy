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

namespace alloy::hal {

// Which transition arms an interrupt. A RUNTIME value, not a template
// parameter: it maps 1:1 onto the trigger-enable bits, and re-arming one pin
// with a different edge must not instantiate a second handler and a second
// callback slot.
enum class pin_edge : std::uint8_t { rising, falling, both };

template <class Inst>
struct exti_impl;

}  // namespace alloy::hal
