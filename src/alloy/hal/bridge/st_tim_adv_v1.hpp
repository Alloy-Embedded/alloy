// The timer:v1 TIM_ADV block (F4/F7 TIM1/TIM8) — SELECTION only.
//
// Codegen includes this file because the IP's curated `class` is `bridge`
// (alloy/hal/<class>/<vendor>_<ip>.hpp). The driver itself — every register
// sequence, the DTG arithmetic, the break path — lives in
// st_tim_adv_body.hpp, which serves both advanced-timer templates and carries
// the argument for why one body is correct for both.

#pragma once

#include <type_traits>

#include "alloy/hal/bridge/st_tim_adv_body.hpp"
#include "alloy/ip/st/tim_adv_v1.hpp"

namespace alloy::hal::detail {

template <>
struct is_adv_bridge_tag<alloy::ip::st::tim_adv_v1> : std::true_type {};

}  // namespace alloy::hal::detail
