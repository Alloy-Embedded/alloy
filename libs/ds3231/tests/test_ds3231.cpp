// Host test for the DS3231 driver against the testkit mock I2C bus — no hardware.
// Verifies the register pointer, the BCD<->binary conversion at the register
// edge, and the datasheet temperature encoding (including a negative value).

#include <cstdint>

#include "alloy_test.hpp"
#include "ds3231.hpp"
#include "testkit/mock_bus.hpp"

using alloy::datetime;
using alloy::lib::ds3231;
using alloy::testkit::mock_i2c;

namespace {
// 2025-07-24 13:45:30, packed BCD as the DS3231 timekeeping registers hold it:
// sec min hour dow date month year.
constexpr std::uint8_t kClock[7] = {0x30, 0x45, 0x13, 0x01, 0x24, 0x07, 0x25};
}  // namespace

ALLOY_TEST(ds3231_now_decodes_bcd_registers) {
    mock_i2c bus;
    bus.queue_read(kClock);

    ds3231 rtc{bus};
    datetime dt;
    ALLOY_CHECK(rtc.now(dt));

    // It addressed 0x68 and set the register pointer to 0x00 first.
    ALLOY_CHECK_EQ(bus.last_addr, 0x68);
    ALLOY_CHECK_EQ(bus.last_write[0], 0x00);
    // BCD was converted to plain binary.
    ALLOY_CHECK_EQ(dt.second, 30);
    ALLOY_CHECK_EQ(dt.minute, 45);
    ALLOY_CHECK_EQ(dt.hour, 13);
    ALLOY_CHECK_EQ(dt.day, 24);
    ALLOY_CHECK_EQ(dt.month, 7);
    ALLOY_CHECK_EQ(dt.year, 25);
}

ALLOY_TEST(ds3231_set_writes_bcd_frame) {
    mock_i2c bus;
    ds3231 rtc{bus};

    datetime dt;
    dt.second = 9;
    dt.minute = 8;
    dt.hour = 23;
    dt.day = 31;
    dt.month = 12;
    dt.year = 99;
    ALLOY_CHECK(rtc.set(dt));

    ALLOY_CHECK_EQ(bus.last_addr, 0x68);
    ALLOY_CHECK_EQ(bus.last_write[0], 0x00);  // starts at the seconds register
    ALLOY_CHECK_EQ(bus.last_write[1], 0x09);  // 9  -> BCD 0x09
    ALLOY_CHECK_EQ(bus.last_write[2], 0x08);  // 8  -> BCD 0x08
    ALLOY_CHECK_EQ(bus.last_write[3], 0x23);  // 23 -> BCD 0x23, bit6=0 (24h)
    ALLOY_CHECK_EQ(bus.last_write[5], 0x31);  // date 31 -> BCD 0x31
    ALLOY_CHECK_EQ(bus.last_write[6], 0x12);  // month 12, century bit clear
    ALLOY_CHECK_EQ(bus.last_write[7], 0x99);  // year 99 -> BCD 0x99
}

ALLOY_TEST(ds3231_read_temperature_positive) {
    mock_i2c bus;
    // Datasheet example: 0x11 = 0x19 (25), 0x12 = 0x40 (fraction 01 -> 0.25).
    std::uint8_t raw[2] = {0x19, 0x40};
    bus.queue_read(raw);

    ds3231 rtc{bus};
    auto t = rtc.read_temperature_c();
    ALLOY_CHECK(t.valid);
    ALLOY_CHECK_EQ(bus.last_write[0], 0x11);  // pointer to the temp MSB register
    ALLOY_CHECK(t.celsius > 25.24f && t.celsius < 25.26f);
}

ALLOY_TEST(ds3231_read_temperature_negative) {
    mock_i2c bus;
    // Datasheet negative encoding: -0.25 C -> 0x11 = 0xFF, 0x12 = 0xC0.
    std::uint8_t raw[2] = {0xFF, 0xC0};
    bus.queue_read(raw);

    ds3231 rtc{bus};
    auto t = rtc.read_temperature_c();
    ALLOY_CHECK(t.valid);
    ALLOY_CHECK(t.celsius > -0.26f && t.celsius < -0.24f);
}

ALLOY_TEST(ds3231_reports_bus_nack) {
    mock_i2c bus;
    bus.fail = true;  // simulate no device on the bus

    ds3231 rtc{bus};
    datetime dt;
    ALLOY_CHECK(!rtc.now(dt));
    ALLOY_CHECK(!rtc.set(dt));
    ALLOY_CHECK(!rtc.read_temperature_c().valid);
}
