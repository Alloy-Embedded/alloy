#pragma once

// drivers/memory/is42s16100f/is42s16100f.hpp
//
// Chip descriptor for the ISSI IS42S16100F-5B SDRAM.
// Written against datasheet revision C (ISSI, April 2017).
// "Chip descriptor" shape (see drivers/README.md): header-only, no bus handle,
// no dynamic allocation. The values here feed a memory-controller init flow
// (e.g. the SAME70 SDRAMC) that the board helper layer owns.
//
// Part summary (IS42S16100F, -5B speed grade):
//   * 16 Mbit organised as 2 banks x 2048 rows x 256 columns x 16 bits
//   * CAS latency = 3 at 166 MHz (-5B)
//   * Auto-refresh period = 64 ms over 2048 rows (31.25 us per row)
//   * 3.3 V, LVTTL IO

#include <cstdint>

namespace alloy::drivers::memory::is42s16100f {

inline constexpr std::uint32_t kBankCount = 2;
inline constexpr std::uint32_t kRowCount = 2048;      // A0..A10
inline constexpr std::uint32_t kColumnCount = 256;    // A0..A7
inline constexpr std::uint32_t kDataBusBits = 16;
inline constexpr std::uint32_t kCapacityBits = 16u * 1024u * 1024u;
inline constexpr std::uint32_t kCasLatencyCycles = 3;

// Datasheet AC timing (ns) for the -5B speed grade (table 12 of rev C).
struct TimingsNs {
    std::uint16_t t_rc;   // ROW cycle time (ACT to ACT same bank): 60 ns
    std::uint16_t t_rcd;  // RAS to CAS delay: 15 ns (min = 1 CLK + 15 ns)
    std::uint16_t t_rp;   // Precharge to ACT: 15 ns
    std::uint16_t t_ras;  // ACT to PRE (same bank): 37 ns min, 120 us max
    std::uint16_t t_wr;   // Write recovery: 2 CLK (encoded as ceiling of 14 ns)
    std::uint16_t t_rfc;  // Auto-refresh cycle: 60 ns
    std::uint32_t refresh_period_ns;  // 31250 ns per row (64 ms / 2048 rows)
};

inline constexpr TimingsNs kTimings{
    .t_rc = 60,
    .t_rcd = 15,
    .t_rp = 15,
    .t_ras = 37,
    .t_wr = 14,
    .t_rfc = 60,
    .refresh_period_ns = 31'250,
};

// Timings expressed in bus clock cycles. Ceil-div keeps every value >= the
// datasheet minimum. The refresh counter value is *cycles per row-refresh
// interval* — exactly the quantity most memory controllers expect.
struct TimingsCycles {
    std::uint16_t t_rc;
    std::uint16_t t_rcd;
    std::uint16_t t_rp;
    std::uint16_t t_ras;
    std::uint16_t t_wr;
    std::uint16_t t_rfc;
    std::uint32_t refresh_period;
};

namespace detail {

[[nodiscard]] inline constexpr auto ceil_ns_to_cycles(std::uint32_t ns,
                                                     std::uint32_t hclk_hz)
    -> std::uint32_t {
    // cycles = ceil(ns * hclk_hz / 1e9)
    const std::uint64_t num = static_cast<std::uint64_t>(ns) * hclk_hz;
    const std::uint64_t denom = 1'000'000'000ull;
    return static_cast<std::uint32_t>((num + denom - 1) / denom);
}

}  // namespace detail

[[nodiscard]] inline constexpr auto timings_for_bus(std::uint32_t hclk_hz)
    -> TimingsCycles {
    return TimingsCycles{
        .t_rc = static_cast<std::uint16_t>(detail::ceil_ns_to_cycles(kTimings.t_rc, hclk_hz)),
        .t_rcd = static_cast<std::uint16_t>(detail::ceil_ns_to_cycles(kTimings.t_rcd, hclk_hz)),
        .t_rp = static_cast<std::uint16_t>(detail::ceil_ns_to_cycles(kTimings.t_rp, hclk_hz)),
        .t_ras = static_cast<std::uint16_t>(detail::ceil_ns_to_cycles(kTimings.t_ras, hclk_hz)),
        .t_wr = static_cast<std::uint16_t>(detail::ceil_ns_to_cycles(kTimings.t_wr, hclk_hz)),
        .t_rfc = static_cast<std::uint16_t>(detail::ceil_ns_to_cycles(kTimings.t_rfc, hclk_hz)),
        .refresh_period =
            detail::ceil_ns_to_cycles(kTimings.refresh_period_ns, hclk_hz),
    };
}

// Compile-time sanity: capacity matches geometry.
static_assert(kBankCount * kRowCount * kColumnCount * kDataBusBits == kCapacityBits,
              "IS42S16100F geometry does not match 16 Mbit capacity");

}  // namespace alloy::drivers::memory::is42s16100f
