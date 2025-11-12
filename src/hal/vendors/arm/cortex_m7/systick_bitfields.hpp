#pragma once

#include <cstdint>

namespace alloy::hal::arm::cortex_m7::systick {

namespace ctrl {
    constexpr uint32_t ENABLE    = (1u << 0);  ///< Counter enable
    constexpr uint32_t TICKINT   = (1u << 1);  ///< Interrupt enable
    constexpr uint32_t CLKSOURCE = (1u << 2);  ///< Clock source (1=CPU, 0=external)
    constexpr uint32_t COUNTFLAG = (1u << 16); ///< Count to zero flag
}

} // namespace alloy::hal::arm::cortex_m7::systick
