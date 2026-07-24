// rtc_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per RTC IP version, constrained on the instance's
// IP tag type. Operations: set(datetime), now() -> datetime.

#pragma once

namespace alloy::hal {

template <class Inst>
struct rtc_impl;

}  // namespace alloy::hal
