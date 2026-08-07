// uptime_us() host-seam contract. The Cortex-M CVR interpolation cannot run
// here (no SysTick block on a host) — that half is proven by the time_probe
// emulation leg. What the host CAN pin is the seam every consumer relies on:
// uptime_us()/1000 == uptime_ms() always (one counter, two views), the µs
// hooks and the pre-existing ms hooks drive the SAME virtual clock, and
// wrap-safe unsigned deltas survive the 2^32 µs wrap — the property the
// Modbus RTU t3.5 silence detector will be written against.

#include "alloy/time.hpp"

#include <cstdint>

#include "alloy_test.hpp"

namespace alloy::test {
void set_uptime_ms(std::uint32_t ms);
void advance_uptime_ms(std::uint32_t d);
void set_uptime_us(std::uint32_t us);
void advance_uptime_us(std::uint32_t d);
}  // namespace alloy::test

ALLOY_TEST(uptime_us_and_ms_are_two_views_of_one_clock) {
    alloy::test::set_uptime_us(0);
    ALLOY_CHECK_EQ(alloy::uptime_us(), 0u);
    ALLOY_CHECK_EQ(alloy::uptime_ms(), 0u);

    alloy::test::advance_uptime_us(999);
    ALLOY_CHECK_EQ(alloy::uptime_us(), 999u);
    ALLOY_CHECK_EQ(alloy::uptime_ms(), 0u);  // sub-tick µs never rounds ms up

    alloy::test::advance_uptime_us(1);
    ALLOY_CHECK_EQ(alloy::uptime_us(), 1'000u);
    ALLOY_CHECK_EQ(alloy::uptime_ms(), 1u);
}

ALLOY_TEST(uptime_ms_hooks_still_drive_the_same_clock) {
    // The ms hooks predate uptime_us(); test_async.cpp depends on them. They
    // must move BOTH views, not a second counter that can drift.
    alloy::test::set_uptime_ms(5);
    ALLOY_CHECK_EQ(alloy::uptime_ms(), 5u);
    ALLOY_CHECK_EQ(alloy::uptime_us(), 5'000u);

    alloy::test::advance_uptime_ms(2);
    ALLOY_CHECK_EQ(alloy::uptime_ms(), 7u);
    ALLOY_CHECK_EQ(alloy::uptime_us(), 7'000u);
}

ALLOY_TEST(uptime_us_delta_survives_the_wrap) {
    // A t3.5 gap measured across the 2^32 µs boundary must read the same as
    // one measured mid-range: unsigned subtraction is the whole contract.
    alloy::test::set_uptime_us(0xFFFF'FFFFu - 500u);
    const std::uint32_t t0 = alloy::uptime_us();
    alloy::test::advance_uptime_us(1'750);  // spec t3.5 above 19200 baud
    const std::uint32_t dt = alloy::uptime_us() - t0;
    ALLOY_CHECK_EQ(dt, 1'750u);
    ALLOY_CHECK(alloy::uptime_us() < t0);  // wrapped — raw compare would lie
}
