// Unit tests for the Tier-1 pure primitives: Result, ring_buffer,
// moving_average, software_timer (incl. counter wraparound), byteorder.

#include <cstdint>

#include "alloy/util/byteorder.hpp"
#include "alloy/util/moving_average.hpp"
#include "alloy/util/result.hpp"
#include "alloy/util/ring_buffer.hpp"
#include "alloy/util/timer.hpp"
#include "alloy_test.hpp"

using namespace alloy;

ALLOY_TEST(result_holds_value) {
    Result<std::uint16_t> r = std::uint16_t{42};
    ALLOY_CHECK(static_cast<bool>(r));
    ALLOY_CHECK(r.has_value());
    ALLOY_CHECK_EQ(*r, 42u);
    ALLOY_CHECK_EQ(r.value(), 42u);
    ALLOY_CHECK_EQ(r.value_or(7u), 42u);
}

ALLOY_TEST(result_holds_error) {
    Result<std::uint16_t> r = error::nack;
    ALLOY_CHECK(!static_cast<bool>(r));
    ALLOY_CHECK(!r.has_value());
    ALLOY_CHECK(r.error() == error::nack);
    ALLOY_CHECK_EQ(r.value_or(7u), 7u);
}

ALLOY_TEST(result_transform_and_propagate) {
    Result<std::uint16_t> ok = std::uint16_t{10};
    auto doubled = ok.transform([](std::uint16_t v) -> std::uint32_t { return v * 2u; });
    ALLOY_CHECK(static_cast<bool>(doubled));
    ALLOY_CHECK_EQ(doubled.value(), 20u);

    Result<std::uint16_t> bad = error::timeout;
    auto still_bad = bad.transform([](std::uint16_t v) -> std::uint32_t { return v * 2u; });
    ALLOY_CHECK(!static_cast<bool>(still_bad));
    ALLOY_CHECK(still_bad.error() == error::timeout);
}

ALLOY_TEST(result_void) {
    Result<void> good = alloy::ok();
    ALLOY_CHECK(static_cast<bool>(good));
    Result<void> bad = error::busy;
    ALLOY_CHECK(!static_cast<bool>(bad));
    ALLOY_CHECK(bad.error() == error::busy);
}

ALLOY_TEST(ring_buffer_fifo_order) {
    ring_buffer<std::uint8_t, 4> rb;
    ALLOY_CHECK(rb.empty());
    ALLOY_CHECK_EQ(rb.capacity(), 4u);
    for (std::uint8_t i = 1; i <= 4; ++i) {
        ALLOY_CHECK(rb.push(i));
    }
    ALLOY_CHECK(rb.full());
    ALLOY_CHECK(!rb.push(99));  // dropped when full
    std::uint8_t out = 0;
    ALLOY_CHECK(rb.pop(out));
    ALLOY_CHECK_EQ(out, 1u);  // FIFO
    ALLOY_CHECK(rb.push(5));   // room again after a pop
    ALLOY_CHECK_EQ(rb.size(), 4u);
}

ALLOY_TEST(ring_buffer_wraps_indices) {
    ring_buffer<std::uint8_t, 3> rb;
    std::uint8_t out = 0;
    for (std::uint8_t round = 0; round < 10; ++round) {
        ALLOY_CHECK(rb.push(round));
        ALLOY_CHECK(rb.pop(out));
        ALLOY_CHECK_EQ(out, round);  // survives head/tail wrap
    }
    ALLOY_CHECK(rb.empty());
    ALLOY_CHECK(!rb.pop(out));
}

ALLOY_TEST(moving_average_smooths) {
    moving_average<std::int16_t, 4, std::int32_t> ma;
    ALLOY_CHECK_EQ(ma.add(100), 100);       // 100/1
    ALLOY_CHECK_EQ(ma.add(200), 150);       // (100+200)/2
    (void)ma.add(300);                      // (100+200+300)/3 = 200
    ALLOY_CHECK_EQ(ma.add(400), 250);       // (100..400)/4
    ALLOY_CHECK(ma.ready());
    ALLOY_CHECK_EQ(ma.add(400), 325);       // window drops the 100: (200+300+400+400)/4
}

ALLOY_TEST(software_timer_expiry) {
    software_timer<std::uint32_t> t{100u};
    t.reset(1000u);
    ALLOY_CHECK(!t.expired(1050u));  // 50 < 100
    ALLOY_CHECK(t.expired(1100u));   // 100 >= 100
    ALLOY_CHECK(t.expired(5000u));
}

ALLOY_TEST(software_timer_periodic_no_drift) {
    software_timer<std::uint32_t> t{100u};
    t.reset(0u);
    ALLOY_CHECK(t.poll(105u));   // fires, phase advances to 100 (not 105)
    ALLOY_CHECK(!t.poll(150u));  // 150-100 = 50 < 100
    ALLOY_CHECK(t.poll(205u));   // 205-100 = 105 >= 100, phase -> 200 (no drift)
    ALLOY_CHECK(!t.poll(250u));
}

ALLOY_TEST(software_timer_survives_wraparound) {
    software_timer<std::uint32_t> t{100u};
    t.reset(0xFFFFFF00u);                 // start near the 32-bit wrap
    ALLOY_CHECK(!t.expired(0xFFFFFF50u));  // 0x50 = 80 elapsed < 100
    ALLOY_CHECK(t.expired(0x00000064u));   // wrapped; unsigned delta = 356 >= 100
}

ALLOY_TEST(byteorder_load_store_roundtrip) {
    std::uint8_t buf[8]{};
    byteorder::store_be16(buf, 0x1234);
    ALLOY_CHECK_EQ(buf[0], 0x12u);
    ALLOY_CHECK_EQ(buf[1], 0x34u);
    ALLOY_CHECK_EQ(byteorder::load_be16(buf), 0x1234u);

    byteorder::store_le32(buf, 0x89ABCDEFu);
    ALLOY_CHECK_EQ(buf[0], 0xEFu);
    ALLOY_CHECK_EQ(buf[3], 0x89u);
    ALLOY_CHECK_EQ(byteorder::load_le32(buf), 0x89ABCDEFu);
    ALLOY_CHECK_EQ(byteorder::load_be32(buf), 0xEFCDAB89u);  // reverse byte order
}

ALLOY_TEST(byteorder_swaps) {
    ALLOY_CHECK_EQ(byteorder::to_big<std::uint16_t>(byteorder::to_big<std::uint16_t>(0xABCD)),
                   0xABCDu);  // swap is its own inverse
    const std::uint32_t v = 0x11223344u;
    ALLOY_CHECK(byteorder::from_little(byteorder::to_little(v)) == v);
    ALLOY_CHECK(byteorder::from_big(byteorder::to_big(v)) == v);
}
