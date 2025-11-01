/// RP2040 SysTick Implementation (stub for now)

#include "core/types.hpp"

namespace alloy::systick::detail {

// Simple microsecond counter (stub)
static volatile core::u64 g_micros = 0;

core::u64 get_micros() {
    return g_micros;
}

void increment_micros(core::u32 delta_us) {
    g_micros += delta_us;
}

} // namespace alloy::systick::detail
