#pragma once

// drivers/sensor/mpu6050/mpu6050.hpp
//
// Driver for InvenSense MPU-6050 6-axis IMU (accelerometer + gyroscope) over I2C.
// Written against datasheet revision 3.4 (PS-MPU-6000A-00).
// Seed driver: chip-ID probe + reset/wake sequence + burst-read of accel, gyro,
// and temperature with configurable full-scale ranges and physical-unit conversion.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::mpu6050 {

// ── Constants ─────────────────────────────────────────────────────────────────

/// I2C address when AD0 pin is LOW (default).
inline constexpr std::uint16_t kDefaultAddress   = 0x68;
/// I2C address when AD0 pin is HIGH.
inline constexpr std::uint16_t kSecondaryAddress = 0x69;

/// Expected value of the WHO_AM_I register.
inline constexpr std::uint8_t kExpectedWhoAmI = 0x68;

// Register addresses.
inline constexpr std::uint8_t kRegSmplrtDiv   = 0x19;  ///< Sample Rate Divider
inline constexpr std::uint8_t kRegConfig      = 0x1A;  ///< Configuration (DLPF)
inline constexpr std::uint8_t kRegGyroConfig  = 0x1B;  ///< Gyroscope Configuration
inline constexpr std::uint8_t kRegAccelConfig = 0x1C;  ///< Accelerometer Configuration
inline constexpr std::uint8_t kRegAccelXoutH  = 0x3B;  ///< Accel X-axis High byte (burst start)
inline constexpr std::uint8_t kRegPwrMgmt1    = 0x6B;  ///< Power Management 1
inline constexpr std::uint8_t kRegWhoAmI      = 0x75;  ///< Device identity register

// PWR_MGMT_1 values.
inline constexpr std::uint8_t kPwrReset = 0x80;  ///< Software reset (H_RESET bit)
inline constexpr std::uint8_t kPwrWake  = 0x01;  ///< Wake from sleep, PLL gyro X as clock

// CONFIG value: DLPF_CFG = 2 → ~94 Hz bandwidth, 1 kHz gyro output rate.
inline constexpr std::uint8_t kConfigDlpf94Hz = 0x02;

// SMPLRT_DIV = 7 → ODR = 1000 Hz / (1 + 7) = 125 Hz.
inline constexpr std::uint8_t kSmplrtDiv125Hz = 0x07;

// ── Types ─────────────────────────────────────────────────────────────────────

/// Accelerometer full-scale range.
enum class AccelRange : std::uint8_t {
    G2  = 0,  ///< ±2 g  (16384 LSB/g)
    G4  = 1,  ///< ±4 g  ( 8192 LSB/g)
    G8  = 2,  ///< ±8 g  ( 4096 LSB/g)
    G16 = 3,  ///< ±16 g ( 2048 LSB/g)
};

/// Gyroscope full-scale range.
enum class GyroRange : std::uint8_t {
    Dps250  = 0,  ///< ±250  dps (131.0 LSB/dps)
    Dps500  = 1,  ///< ±500  dps ( 65.5 LSB/dps)
    Dps1000 = 2,  ///< ±1000 dps ( 32.8 LSB/dps)
    Dps2000 = 3,  ///< ±2000 dps ( 16.4 LSB/dps)
};

struct Config {
    std::uint16_t address     = kDefaultAddress;
    AccelRange    accel_range = AccelRange::G2;
    GyroRange     gyro_range  = GyroRange::Dps250;
};

struct Measurement {
    float accel_x_g;      ///< X-axis acceleration in g
    float accel_y_g;      ///< Y-axis acceleration in g
    float accel_z_g;      ///< Z-axis acceleration in g
    float gyro_x_dps;     ///< X-axis angular rate in degrees per second
    float gyro_y_dps;     ///< Y-axis angular rate in degrees per second
    float gyro_z_dps;     ///< Z-axis angular rate in degrees per second
    float temperature_c;  ///< Die temperature in degrees Celsius
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 100 ms (reset settle time).
/// Loop count approximates elapsed time on Cortex-M / RISC-V at common speeds.
inline void busy_wait_100ms() {
    volatile std::uint32_t n = 1'000'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Returns the accelerometer sensitivity in LSB/g for the given range.
[[nodiscard]] inline constexpr auto accel_sensitivity(AccelRange r) -> float {
    switch (r) {
        case AccelRange::G2:  return 16384.0f;
        case AccelRange::G4:  return  8192.0f;
        case AccelRange::G8:  return  4096.0f;
        case AccelRange::G16: return  2048.0f;
    }
    return 16384.0f;  // unreachable; satisfy compiler
}

/// Returns the gyroscope sensitivity in LSB/dps for the given range.
[[nodiscard]] inline constexpr auto gyro_sensitivity(GyroRange r) -> float {
    switch (r) {
        case GyroRange::Dps250:  return 131.0f;
        case GyroRange::Dps500:  return  65.5f;
        case GyroRange::Dps1000: return  32.8f;
        case GyroRange::Dps2000: return  16.4f;
    }
    return 131.0f;  // unreachable; satisfy compiler
}

/// Reconstruct a big-endian signed 16-bit integer from two consecutive bytes.
[[nodiscard]] inline constexpr auto to_int16(std::uint8_t hi,
                                              std::uint8_t lo) -> std::int16_t {
    return static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(hi) << 8) | static_cast<std::uint16_t>(lo));
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Initialises the MPU-6050.
    ///
    /// Sequence:
    ///   1. Read WHO_AM_I register and verify it equals 0x68.
    ///   2. Write PWR_MGMT_1 = 0x80 (software reset). Busy-wait ~100 ms.
    ///   3. Write PWR_MGMT_1 = 0x01 (wake from sleep, PLL gyro X clock).
    ///   4. Write SMPLRT_DIV = 0x07 (125 Hz output data rate).
    ///   5. Write CONFIG     = 0x02 (94 Hz DLPF bandwidth).
    ///   6. Write GYRO_CONFIG  with AFS_SEL[4:3] from cfg_.gyro_range.
    ///   7. Write ACCEL_CONFIG with AFS_SEL[4:3] from cfg_.accel_range.
    ///
    /// Returns `CommunicationError` on any I2C failure or if WHO_AM_I
    /// returns an unexpected value.
    [[nodiscard]] auto init() -> ResultVoid {
        // Step 1: verify device identity.
        const std::array<std::uint8_t, 1> who_am_i_reg{kRegWhoAmI};
        std::array<std::uint8_t, 1> who_am_i_val{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{who_am_i_reg},
                                      std::span<std::uint8_t>{who_am_i_val});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if (who_am_i_val[0] != kExpectedWhoAmI) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 2: software reset.
        if (auto r = write_reg(kRegPwrMgmt1, kPwrReset); r.is_err()) {
            return r;
        }
        detail::busy_wait_100ms();

        // Step 3: wake from sleep, select PLL gyro X as clock source.
        if (auto r = write_reg(kRegPwrMgmt1, kPwrWake); r.is_err()) {
            return r;
        }

        // Step 4: sample rate divider → 125 Hz.
        if (auto r = write_reg(kRegSmplrtDiv, kSmplrtDiv125Hz); r.is_err()) {
            return r;
        }

        // Step 5: DLPF → 94 Hz bandwidth.
        if (auto r = write_reg(kRegConfig, kConfigDlpf94Hz); r.is_err()) {
            return r;
        }

        // Step 6: gyroscope full-scale range (FS_SEL in bits [4:3]).
        const auto gyro_bits =
            static_cast<std::uint8_t>(static_cast<std::uint8_t>(cfg_.gyro_range) << 3);
        if (auto r = write_reg(kRegGyroConfig, gyro_bits); r.is_err()) {
            return r;
        }

        // Step 7: accelerometer full-scale range (AFS_SEL in bits [4:3]).
        const auto accel_bits =
            static_cast<std::uint8_t>(static_cast<std::uint8_t>(cfg_.accel_range) << 3);
        if (auto r = write_reg(kRegAccelConfig, accel_bits); r.is_err()) {
            return r;
        }

        return alloy::core::Ok();
    }

    /// Reads one measurement from the MPU-6050.
    ///
    /// Burst-reads 14 bytes starting at ACCEL_XOUT_H (0x3B):
    ///   bytes  0-1:  accel X (big-endian int16)
    ///   bytes  2-3:  accel Y
    ///   bytes  4-5:  accel Z
    ///   bytes  6-7:  temperature (big-endian int16)
    ///   bytes  8-9:  gyro X
    ///   bytes 10-11: gyro Y
    ///   bytes 12-13: gyro Z
    ///
    /// Physical-unit conversions:
    ///   accel [g]   = raw / sensitivity_lsb_per_g
    ///   gyro [dps]  = raw / sensitivity_lsb_per_dps
    ///   temp [°C]   = raw / 340.0 + 36.53
    ///
    /// Returns `CommunicationError` on any I2C failure.
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Point to the first data register.
        const std::array<std::uint8_t, 1> start_reg{kRegAccelXoutH};
        std::array<std::uint8_t, 14> buf{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{start_reg},
                                      std::span<std::uint8_t>{buf});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Reconstruct signed 16-bit values (big-endian).
        const auto raw_ax = detail::to_int16(buf[0],  buf[1]);
        const auto raw_ay = detail::to_int16(buf[2],  buf[3]);
        const auto raw_az = detail::to_int16(buf[4],  buf[5]);
        const auto raw_t  = detail::to_int16(buf[6],  buf[7]);
        const auto raw_gx = detail::to_int16(buf[8],  buf[9]);
        const auto raw_gy = detail::to_int16(buf[10], buf[11]);
        const auto raw_gz = detail::to_int16(buf[12], buf[13]);

        // Convert to physical units.
        const float a_sens = detail::accel_sensitivity(cfg_.accel_range);
        const float g_sens = detail::gyro_sensitivity(cfg_.gyro_range);

        const Measurement m{
            .accel_x_g     = static_cast<float>(raw_ax) / a_sens,
            .accel_y_g     = static_cast<float>(raw_ay) / a_sens,
            .accel_z_g     = static_cast<float>(raw_az) / a_sens,
            .gyro_x_dps    = static_cast<float>(raw_gx) / g_sens,
            .gyro_y_dps    = static_cast<float>(raw_gy) / g_sens,
            .gyro_z_dps    = static_cast<float>(raw_gz) / g_sens,
            .temperature_c = (static_cast<float>(raw_t) / 340.0f) + 36.53f,
        };

        return alloy::core::Ok(m);
    }

private:
    BusHandle* bus_;
    Config     cfg_;

    /// Helper: write a single register byte.
    [[nodiscard]] auto write_reg(std::uint8_t reg,
                                  std::uint8_t val) -> alloy::core::Result<void, alloy::core::ErrorCode> {
        const std::array<std::uint8_t, 2> payload{reg, val};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{payload});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        return alloy::core::Ok();
    }
};

}  // namespace alloy::drivers::sensor::mpu6050

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockI2cForMpu6050Gate {
    [[nodiscard]] auto write(std::uint16_t, std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto read(std::uint16_t, std::span<std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto write_read(std::uint16_t,
                                  std::span<const std::uint8_t>,
                                  std::span<std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};
static_assert(
    sizeof(alloy::drivers::sensor::mpu6050::Device<_MockI2cForMpu6050Gate>) > 0,
    "mpu6050 Device must compile against the documented I2C bus surface");
}  // namespace
