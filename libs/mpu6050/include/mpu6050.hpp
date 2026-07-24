// InvenSense MPU-6050 6-axis IMU (3-axis accelerometer + 3-axis gyroscope) over
// I2C.
//
// Constrained ONLY on the alloy bus concept — no chip, no family, no board owns
// a register address at the framework's call sites. Behavior from the InvenSense
// MPU-6000/MPU-6050 register map (RM-MPU-6000A-00, rev 4.2): after power-on the
// device sits in sleep, so wake() clears PWR_MGMT_1 (0x6B). WHO_AM_I (0x75)
// returns the fixed 0x68 identity (the I2C address with the AD0 bit masked). A
// burst read from ACCEL_XOUT_H (0x3B) yields six big-endian int16 accel words;
// GYRO_XOUT_H (0x43) yields six big-endian int16 gyro words. Raw counts are
// returned verbatim — the caller scales with the constexpr sensitivities for the
// power-on default ranges (accel +/-2 g, gyro +/-250 deg/s).

#pragma once

#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::lib {

template <class Bus>
    requires alloy::I2cBus<Bus>
class mpu6050 {
public:
    static constexpr std::uint8_t addr_low = 0x68;   // AD0 pin tied to GND
    static constexpr std::uint8_t addr_high = 0x69;  // AD0 pin tied to VDD

    // Fixed WHO_AM_I identity: the 7-bit address with the AD0 bit (bit 0)
    // masked, i.e. always 0x68 regardless of the AD0 strap.
    static constexpr std::uint8_t who_am_i = 0x68;

    // Power-on default full-scale sensitivities (datasheet section 6.2/6.3).
    // Divide the raw int16 counts by these to get physical units.
    static constexpr float accel_lsb_per_g = 16384.0f;      // +/-2 g range
    static constexpr float gyro_lsb_per_dps = 131.0f;       // +/-250 deg/s range

    struct sample {
        std::int16_t ax = 0;
        std::int16_t ay = 0;
        std::int16_t az = 0;
        std::int16_t gx = 0;
        std::int16_t gy = 0;
        std::int16_t gz = 0;
        bool valid = false;  // false on any NACK / bus error — never trust values
    };

    constexpr explicit mpu6050(const Bus& bus, std::uint8_t address = addr_low)
        : bus_(bus), addr_(address) {}
    mpu6050(const Bus&&, std::uint8_t = addr_low) = delete;  // no dangling bus

    // Read WHO_AM_I (0x75). Returns 0x68 on a healthy MPU-6050; check against
    // who_am_i. On a bus error the out-param is left untouched and false is
    // returned.
    [[nodiscard]] bool identify(std::uint8_t& id) const {
        std::uint8_t v = 0;
        if (!read_reg(kRegWhoAmI, {&v, 1})) {
            return false;
        }
        id = v;
        return true;
    }

    // Clear PWR_MGMT_1 (0x6B) to wake the device from its power-on sleep state
    // (also selects the internal 8 MHz oscillator). Returns false on NACK.
    [[nodiscard]] bool wake() const {
        const std::uint8_t cmd[2] = {kRegPwrMgmt1, 0x00};
        return bus_.write(addr_, cmd);
    }

    // Burst-read the six accelerometer bytes at ACCEL_XOUT_H (0x3B) and the six
    // gyroscope bytes at GYRO_XOUT_H (0x43), decoding each big-endian int16.
    // valid is false if either burst NACKs.
    [[nodiscard]] sample read() const {
        std::uint8_t a[6] = {};
        std::uint8_t g[6] = {};
        if (!read_reg(kRegAccelXoutH, a) || !read_reg(kRegGyroXoutH, g)) {
            return {};
        }
        sample s;
        s.ax = be16(a[0], a[1]);
        s.ay = be16(a[2], a[3]);
        s.az = be16(a[4], a[5]);
        s.gx = be16(g[0], g[1]);
        s.gy = be16(g[2], g[3]);
        s.gz = be16(g[4], g[5]);
        s.valid = true;
        return s;
    }

private:
    static constexpr std::uint8_t kRegAccelXoutH = 0x3B;
    static constexpr std::uint8_t kRegGyroXoutH = 0x43;
    static constexpr std::uint8_t kRegPwrMgmt1 = 0x6B;
    static constexpr std::uint8_t kRegWhoAmI = 0x75;

    // Register read: write the register pointer, then read len bytes.
    [[nodiscard]] bool read_reg(std::uint8_t reg, std::span<std::uint8_t> out) const {
        const std::uint8_t ptr[1] = {reg};
        return bus_.write_read(addr_, ptr, out);
    }

    static std::int16_t be16(std::uint8_t hi, std::uint8_t lo) {
        return static_cast<std::int16_t>(
            static_cast<std::uint16_t>((static_cast<std::uint16_t>(hi) << 8) | lo));
    }

    const Bus& bus_;
    std::uint8_t addr_;
};

}  // namespace alloy::lib
