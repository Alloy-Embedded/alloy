// wwdt_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per WINDOW watchdog IP, constrained on the
// instance's IP tag type, exactly like wdt_impl<Inst> next door. The two are
// separate templates because the two peripherals are separate blocks with
// different contracts, not two versions of one thing: an IWDG has no early
// bound to program and no interrupt to hook, and a facade that pretended
// otherwise would have to lie in one direction or the other.

#pragma once

namespace alloy::hal {

template <class Inst>
struct wwdt_impl;

}  // namespace alloy::hal
