// pin_impl<Pin> — primary template, intentionally undefined.
//
// One partial specialization exists per GPIO IP version (st_gpio_v2.hpp, ...),
// constrained on the pin's port IP tag type. Using a pin whose port IP has no
// driver is a compile error pointing here.

#pragma once

namespace alloy::hal {

template <class Pin>
struct pin_impl;

}  // namespace alloy::hal
