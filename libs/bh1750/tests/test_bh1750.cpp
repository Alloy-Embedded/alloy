// Host test for the BH1750 driver against the testkit mock I2C bus — no
// hardware. Verifies the opcode sequence, the datasheet lux conversion, the
// conversion wait, the alternate address, and honest NACK reporting.

#include <cstdint>

#include "alloy_test.hpp"
#include "bh1750.hpp"
#include "testkit/mock_bus.hpp"

using alloy::lib::bh1750;
using alloy::testkit::mock_delay;
using alloy::testkit::mock_i2c;

// Datasheet worked example (BH1750FVI Rev.D): a raw count of 0x2E90 = 11920 in
// continuous H-resolution mode decodes to 11920 / 1.2 = 9933.3 lx.
namespace {
constexpr std::uint8_t kFrame[2] = {0x2E, 0x90};
}

ALLOY_TEST(bh1750_measure_decodes_a_valid_frame) {
    mock_i2c bus;
    mock_delay delay;
    bus.queue_read(kFrame);

    bh1750 sensor{bus, delay};
    auto r = sensor.measure();

    ALLOY_CHECK(r.valid);
    // Two commands issued to the default 0x23 address: power-on then cont H-res.
    ALLOY_CHECK_EQ(bus.last_addr, 0x23);
    ALLOY_CHECK_EQ(bus.write_count, 2u);
    ALLOY_CHECK_EQ(bus.last_write[0], 0x10);  // last write is the mode opcode
    // And waited for the (max) conversion time.
    ALLOY_CHECK(delay.total_ns >= 180'000'000u);
    // Value within rounding of the datasheet formula (11920 / 1.2 = 9933.3).
    ALLOY_CHECK(r.lux > 9933.0f && r.lux < 9934.0f);
}

ALLOY_TEST(bh1750_reports_bus_nack) {
    mock_i2c bus;
    mock_delay delay;
    bus.fail = true;  // simulate no device on the bus

    bh1750 sensor{bus, delay};
    ALLOY_CHECK(!sensor.measure().valid);
    ALLOY_CHECK(!sensor.power_down());
}

ALLOY_TEST(bh1750_honors_the_alternate_address) {
    mock_i2c bus;
    mock_delay delay;
    bus.queue_read(kFrame);

    bh1750 sensor{bus, delay, bh1750<mock_i2c, mock_delay>::addr_high};
    (void)sensor.measure();
    ALLOY_CHECK_EQ(bus.last_addr, 0x5C);
}
