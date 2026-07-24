// Host test for the MPU-6050 driver against the testkit mock I2C bus — no
// hardware. Verifies the WHO_AM_I identity, the wake sequence, the register
// pointer writes, and big-endian signed decoding of the accel + gyro bursts.

#include <cstdint>

#include "alloy_test.hpp"
#include "mpu6050.hpp"
#include "testkit/mock_bus.hpp"

using alloy::lib::mpu6050;
using alloy::testkit::mock_i2c;

ALLOY_TEST(mpu6050_identify_returns_the_datasheet_who_am_i) {
    mock_i2c bus;
    const std::uint8_t id_frame[1] = {0x68};  // fixed MPU-6050 identity
    bus.queue_read(id_frame);

    mpu6050 imu{bus};
    std::uint8_t id = 0;
    ALLOY_CHECK(imu.identify(id));
    ALLOY_CHECK_EQ(id, mpu6050<mock_i2c>::who_am_i);
    ALLOY_CHECK_EQ(id, 0x68);
    // It addressed 0x68 and pointed at WHO_AM_I (0x75).
    ALLOY_CHECK_EQ(bus.last_addr, 0x68);
    ALLOY_CHECK_EQ(bus.last_write[0], 0x75);
}

ALLOY_TEST(mpu6050_wake_clears_pwr_mgmt_1) {
    mock_i2c bus;

    mpu6050 imu{bus};
    ALLOY_CHECK(imu.wake());
    ALLOY_CHECK_EQ(bus.last_addr, 0x68);
    ALLOY_CHECK_EQ(bus.last_write[0], 0x6B);  // PWR_MGMT_1
    ALLOY_CHECK_EQ(bus.last_write[1], 0x00);  // wake + internal oscillator
}

ALLOY_TEST(mpu6050_read_decodes_big_endian_signed_words) {
    mock_i2c bus;
    // Accel: ax=+1g (0x4000=16384), ay=0, az=-1g (0xC000 = -16384).
    // Gyro:  gx=0, gy=+250dps (0x0083=131), gz=-131 (0xFF7D).
    const std::uint8_t accel[6] = {0x40, 0x00, 0x00, 0x00, 0xC0, 0x00};
    const std::uint8_t gyro[6] = {0x00, 0x00, 0x00, 0x83, 0xFF, 0x7D};
    bus.queue_read(accel);
    bus.queue_read(gyro);

    mpu6050 imu{bus};
    auto s = imu.read();

    ALLOY_CHECK(s.valid);
    ALLOY_CHECK_EQ(s.ax, 16384);
    ALLOY_CHECK_EQ(s.ay, 0);
    ALLOY_CHECK_EQ(s.az, -16384);
    ALLOY_CHECK_EQ(s.gx, 0);
    ALLOY_CHECK_EQ(s.gy, 131);
    ALLOY_CHECK_EQ(s.gz, -131);
    // Last burst pointed at GYRO_XOUT_H (0x43).
    ALLOY_CHECK_EQ(bus.last_write[0], 0x43);
}

ALLOY_TEST(mpu6050_reports_bus_nack) {
    mock_i2c bus;
    bus.fail = true;  // no device on the bus

    mpu6050 imu{bus};
    std::uint8_t id = 0xAA;
    ALLOY_CHECK(!imu.identify(id));
    ALLOY_CHECK_EQ(id, 0xAA);  // out-param untouched on failure
    ALLOY_CHECK(!imu.wake());
    ALLOY_CHECK(!imu.read().valid);
}

ALLOY_TEST(mpu6050_honors_the_alternate_address) {
    mock_i2c bus;
    const std::uint8_t id_frame[1] = {0x68};
    bus.queue_read(id_frame);

    mpu6050 imu{bus, mpu6050<mock_i2c>::addr_high};
    std::uint8_t id = 0;
    (void)imu.identify(id);
    ALLOY_CHECK_EQ(bus.last_addr, 0x69);
}
