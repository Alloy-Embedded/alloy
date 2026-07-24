// Host test for the BME280 driver against the testkit mock I2C bus — no hardware.
// Verifies the init/chip-id handshake, the forced-mode measurement sequence, and
// the datasheet integer compensation.
//
// The calibration + raw ADC words below are the worked example from the Bosch
// BMP280/BME280 datasheet (dig_T1=27504, dig_T2=26435, dig_T3=-1000, dig_P1..P9
// as encoded, adc_T=519888, adc_P=415148), which the datasheet documents as
// ~25.04 °C / ~100696 Pa. Humidity trim (dig_H1..H6) is a representative set;
// adc_H=0x7530 decodes to ~59.6 %RH through bme280_compensate_H_int32.

#include <cstdint>

#include "alloy_test.hpp"
#include "bme280.hpp"
#include "testkit/mock_bus.hpp"

using alloy::lib::bme280;
using alloy::testkit::mock_delay;
using alloy::testkit::mock_i2c;

namespace {
constexpr std::uint8_t kChipId[1] = {0x60};

// 0x88..0xA1: dig_T1..T3, dig_P1..P9, reserved, dig_H1 (little-endian words).
constexpr std::uint8_t kCalib0[26] = {
    0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC,  // T1=27504, T2=26435, T3=-1000
    0x7D, 0x8E, 0x43, 0xD6, 0xD0, 0x0B,  // P1=36477, P2=-10685, P3=3024
    0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF,  // P4=2855, P5=140, P6=-7
    0x8C, 0x3C, 0xF8, 0xC7, 0x70, 0x17,  // P7=15500, P8=-14600, P9=6000
    0x00, 0x4B,                          // reserved(0xA0), H1=75
};
// 0xE1..0xE7: dig_H2, dig_H3, dig_H4/H5 (nibble-split), dig_H6.
constexpr std::uint8_t kCalib1[7] = {
    0x6A, 0x01, 0x00, 0x12, 0x2C, 0x03, 0x1E,
};
// 0xF7..0xFE burst: press(msb,lsb,xlsb), temp(msb,lsb,xlsb), hum(msb,lsb).
// adc_P=415148, adc_T=519888, adc_H=30000.
constexpr std::uint8_t kData[8] = {
    0x65, 0x5A, 0xC0, 0x7E, 0xE5, 0x00, 0x75, 0x30,
};

void queue_full_session(mock_i2c& bus) {
    bus.queue_read(kChipId);
    bus.queue_read(kCalib0);
    bus.queue_read(kCalib1);
    bus.queue_read(kData);
}
}  // namespace

ALLOY_TEST(bme280_init_and_measure_decode_datasheet_frame) {
    mock_i2c bus;
    mock_delay delay;
    queue_full_session(bus);

    bme280 sensor{bus, delay};
    ALLOY_CHECK(sensor.init());  // chip-id 0x60 accepted, calibration latched

    auto r = sensor.measure();
    ALLOY_CHECK(r.valid);
    // Talked to the default (primary) address.
    ALLOY_CHECK_EQ(bus.last_addr, 0x76);
    // Last transaction was the burst read from the data register 0xF7.
    ALLOY_CHECK_EQ(bus.last_write[0], 0xF7);
    // Waited the worst-case forced conversion time.
    ALLOY_CHECK(delay.total_ns >= 9'300'000u);
    // Datasheet-derived values: ~25.08 C, ~100.7 kPa, ~59.6 %RH.
    ALLOY_CHECK(r.temperature_c > 25.0f && r.temperature_c < 25.1f);
    ALLOY_CHECK(r.pressure_pa > 100'600.0f && r.pressure_pa < 100'800.0f);
    ALLOY_CHECK(r.humidity_pct > 59.0f && r.humidity_pct < 60.0f);
}

ALLOY_TEST(bme280_rejects_a_wrong_chip_id) {
    mock_i2c bus;
    mock_delay delay;
    std::uint8_t wrong_id[1] = {0x58};  // e.g. a BMP280 id, not a BME280
    bus.queue_read(wrong_id);

    bme280 sensor{bus, delay};
    ALLOY_CHECK(!sensor.init());          // must refuse the wrong device
    ALLOY_CHECK(!sensor.measure().valid);  // and stay unarmed
}

ALLOY_TEST(bme280_measure_before_init_is_invalid) {
    mock_i2c bus;
    mock_delay delay;
    bus.queue_read(kData);  // even with data waiting, no calibration yet

    bme280 sensor{bus, delay};
    ALLOY_CHECK(!sensor.measure().valid);
}

ALLOY_TEST(bme280_reports_bus_nack) {
    mock_i2c bus;
    mock_delay delay;
    bus.fail = true;  // simulate no device on the bus

    bme280 sensor{bus, delay};
    ALLOY_CHECK(!sensor.init());
    ALLOY_CHECK(!sensor.measure().valid);
    ALLOY_CHECK(!sensor.reset());
}

ALLOY_TEST(bme280_honors_the_alternate_address) {
    mock_i2c bus;
    mock_delay delay;
    queue_full_session(bus);

    bme280 sensor{bus, delay, bme280<mock_i2c, mock_delay>::addr_alt};
    ALLOY_CHECK(sensor.init());
    (void)sensor.measure();
    ALLOY_CHECK_EQ(bus.last_addr, 0x77);
}
