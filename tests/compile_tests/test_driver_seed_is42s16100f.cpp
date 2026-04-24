// Compile test: IS42S16100F-5B chip descriptor. There is no bus handle — the
// entry is a pure constexpr descriptor consumed by the board's SDRAMC init
// code. We static_assert the datasheet geometry and spot-check the ns→cycles
// helper at a realistic 150 MHz SDRAMC bus clock.

#include <cstdint>

#include "drivers/memory/is42s16100f/is42s16100f.hpp"

namespace {

namespace ds = alloy::drivers::memory::is42s16100f;

static_assert(ds::kBankCount == 2);
static_assert(ds::kRowCount == 2048);
static_assert(ds::kColumnCount == 256);
static_assert(ds::kDataBusBits == 16);
static_assert(ds::kCasLatencyCycles == 3);
static_assert(ds::kCapacityBits == 16u * 1024u * 1024u);

// At 150 MHz each cycle is ~6.67 ns. Ceil-div must keep each timing >= the
// datasheet minimum.
constexpr auto kTimings150 = ds::timings_for_bus(150'000'000u);
static_assert(kTimings150.t_rc >= 9);   // 60 ns / 6.67 ns = 9
static_assert(kTimings150.t_rcd >= 3);  // 15 ns / 6.67 ns = 3
static_assert(kTimings150.t_rp >= 3);
static_assert(kTimings150.t_ras >= 6);
static_assert(kTimings150.t_wr >= 3);
static_assert(kTimings150.t_rfc >= 9);
static_assert(kTimings150.refresh_period >= 4687);  // 31250 / 6.67

[[maybe_unused]] constexpr auto kTimings100 = ds::timings_for_bus(100'000'000u);
static_assert(kTimings100.t_rc >= 6);
static_assert(kTimings100.refresh_period >= 3125);

}  // namespace
