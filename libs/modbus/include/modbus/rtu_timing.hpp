// RTU inter-frame timing facts, computed from the baud rate at compile time
// where the baud is constexpr (the usual case: it comes from board data).
//
// The spec (Modbus over Serial Line v1.02, §2.5.1.1) frames RTU by SILENCE:
// t1.5 is the maximum intra-frame gap between characters, t3.5 the minimum
// inter-frame gap. A character is 11 bit times (start + 8 data + parity/2nd
// stop + stop) REGARDLESS of the actual parity setting — the spec fixes the
// arithmetic so both ends compute the same windows. Above 19200 baud the spec
// abandons the multiplication and pins t1.5 = 750 µs, t3.5 = 1750 µs, because
// scaled values become too tight for real UART FIFOs — that clamp is why even
// 115200-baud RTU needs only microsecond (not nanosecond) timing.
//
// alloy::uptime_us() has 1 µs granularity interpolated from a 1 ms tick, so
// at ≤19200 baud (t3.5 ≥ 2005 µs) silence detection is solid; at higher bauds
// the 1750 µs window still exceeds a full tick, but the sub-tick error margin
// matters — the client/server layers state this in their support claims.

#pragma once

#include <cstdint>

namespace alloy::lib::modbus {

struct rtu_times {
    std::uint32_t t1_5_us;  // max silence WITHIN a frame
    std::uint32_t t3_5_us;  // min silence BETWEEN frames
};

[[nodiscard]] constexpr rtu_times rtu_times_for(std::uint32_t baud) {
    if (baud > 19'200u) {
        return {.t1_5_us = 750u, .t3_5_us = 1'750u};  // spec clamp
    }
    // 11 bits/char, in µs, rounded UP — a detector must never fire early.
    const std::uint32_t char_us = (11u * 1'000'000u + baud - 1u) / baud;
    return {.t1_5_us = char_us + char_us / 2u, .t3_5_us = char_us * 3u + char_us / 2u};
}

// The two rates every field bus actually runs at, pinned by the compiler so
// the rounding direction can never regress silently.
static_assert(rtu_times_for(9'600).t3_5_us == 4'011u);
static_assert(rtu_times_for(19'200).t3_5_us == 2'005u);
static_assert(rtu_times_for(115'200).t3_5_us == 1'750u);  // clamped

}  // namespace alloy::lib::modbus
