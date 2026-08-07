// CRC-16/MODBUS: golden vectors verified against an independent
// implementation (and the spec's own example frame), the incremental
// accumulator against the one-shot form, and the little-endian wire order —
// the one field in Modbus that is NOT big-endian, and therefore the one a
// fresh implementation gets backwards.

#include "modbus/crc.hpp"

#include <cstdint>

#include "alloy_test.hpp"

namespace {
using namespace alloy::lib::modbus;
}

ALLOY_TEST(modbus_crc_matches_the_catalogue_and_the_spec_example) {
    const std::uint8_t check[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    ALLOY_CHECK_EQ(crc16(check), 0x4B37u);

    // Request "unit 0x11, read 3 holding regs @0x006B" — the worked example
    // in the Modbus serial line spec. CRC value 0x8776, wire order 76 87.
    const std::uint8_t spec[6] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03};
    ALLOY_CHECK_EQ(crc16(spec), 0x8776u);

    const std::uint8_t fc03[6] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
    ALLOY_CHECK_EQ(crc16(fc03), 0x0A84u);
}

ALLOY_TEST(modbus_crc_accumulator_equals_the_one_shot_form) {
    const std::uint8_t data[6] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03};
    crc16_accumulator acc;
    for (std::uint8_t b : data) {
        acc.update(b);
    }
    ALLOY_CHECK_EQ(acc.value(), crc16(data));

    acc.reset();
    ALLOY_CHECK_EQ(acc.value(), 0xFFFFu);  // init value, empty input
}

ALLOY_TEST(modbus_crc_append_writes_lo_byte_first_and_check_round_trips) {
    std::uint8_t frame[8] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03, 0, 0};
    const std::size_t total = append_crc(frame, 6);
    ALLOY_CHECK_EQ(total, 8u);
    ALLOY_CHECK_EQ(frame[6], 0x76u);  // lo first — RTU wire order
    ALLOY_CHECK_EQ(frame[7], 0x87u);
    ALLOY_CHECK(crc_ok(frame));

    frame[3] ^= 0x01u;  // flip one payload bit
    ALLOY_CHECK(!crc_ok(frame));
}
