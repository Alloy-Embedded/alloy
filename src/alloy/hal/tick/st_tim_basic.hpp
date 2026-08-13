// Time-base driver binding for the ST `tim_basic` block.
//
// Codegen includes this file because the IP's curated `class` is `tick`
// (alloy/hal/<class>/<vendor>_<ip>.hpp). The implementation is shared with the
// other three ST timer blocks — see st_timebase.hpp for why that sharing is a
// fact about the silicon rather than a shortcut.

#pragma once

#include <concepts>

#include "alloy/hal/tick/st_timebase.hpp"
#include "alloy/hal/tick/tick_impl.hpp"
#include "alloy/ip/st/tim_basic.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::tim_basic>
struct tick_impl<Inst> : detail::st_tick_timebase<Inst> {};

}  // namespace alloy::hal
