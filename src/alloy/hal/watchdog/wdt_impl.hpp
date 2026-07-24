// wdt_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per watchdog IP version, constrained on the
// instance's IP tag type (data-driven driver selection).

#pragma once

namespace alloy::hal {

template <class Inst>
struct wdt_impl;

}  // namespace alloy::hal
