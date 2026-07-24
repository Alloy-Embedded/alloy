// Host test for the SHT31 driver against the testkit mock I2C bus — no hardware.
// Verifies the command sequence, CRC validation, and the datasheet conversion.

#include <cstdint>

#include "alloy_test.hpp"
#include "sht31.hpp"
#include "testkit/mock_bus.hpp"

using alloy::lib::sht31;
using alloy::testkit::mock_delay;
using alloy::testkit::mock_i2c;

// Sensirion CRC-8 known-answer vector from the datasheet: crc(0xBE,0xEF) = 0x92.
// Reusing 0xBEEF for both temp and humidity gives raw = 48879 ->
//   T = -45 + 175*48879/65535 = 85.53 C,  RH = 100*48879/65535 = 74.58 %.
namespace {
constexpr std::uint8_t kFrame[6] = {0xBE, 0xEF, 0x92, 0xBE, 0xEF, 0x92};
}

ALLOY_TEST(sht31_measure_decodes_a_valid_frame) {
    mock_i2c bus;
    mock_delay delay;
    bus.queue_read(kFrame);

    sht31 sensor{bus, delay};
    auto r = sensor.measure();

    ALLOY_CHECK(r.valid);
    // It issued the high-rep single-shot command 0x2400 to 0x44.
    ALLOY_CHECK_EQ(bus.last_addr, 0x44);
    ALLOY_CHECK_EQ(bus.last_write[0], 0x24);
    ALLOY_CHECK_EQ(bus.last_write[1], 0x00);
    // And waited for the conversion.
    ALLOY_CHECK(delay.total_ns >= 15'000'000u);
    // Values within rounding of the datasheet formula.
    ALLOY_CHECK(r.temperature_c > 85.0f && r.temperature_c < 86.0f);
    ALLOY_CHECK(r.humidity_pct > 74.0f && r.humidity_pct < 75.0f);
}

ALLOY_TEST(sht31_rejects_a_bad_crc) {
    mock_i2c bus;
    mock_delay delay;
    std::uint8_t bad[6] = {0xBE, 0xEF, 0x00, 0xBE, 0xEF, 0x92};  // temp CRC wrong
    bus.queue_read(bad);

    sht31 sensor{bus, delay};
    auto r = sensor.measure();
    ALLOY_CHECK(!r.valid);  // corrupt reading must not be trusted
}

ALLOY_TEST(sht31_reports_bus_nack) {
    mock_i2c bus;
    mock_delay delay;
    bus.fail = true;  // simulate no device on the bus

    sht31 sensor{bus, delay};
    ALLOY_CHECK(!sensor.measure().valid);
    ALLOY_CHECK(!sensor.reset());
}

ALLOY_TEST(sht31_honors_the_alternate_address) {
    mock_i2c bus;
    mock_delay delay;
    bus.queue_read(kFrame);

    sht31 sensor{bus, delay, sht31<mock_i2c, mock_delay>::addr_high};
    (void)sensor.measure();
    ALLOY_CHECK_EQ(bus.last_addr, 0x45);
}
