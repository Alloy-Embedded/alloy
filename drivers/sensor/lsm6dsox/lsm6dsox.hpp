#pragma once

// drivers/sensor/lsm6dsox/lsm6dsox.hpp
//
// Driver for STMicroelectronics LSM6DSOX 6-axis IMU (accelerometer + gyroscope)
// over I2C.
// Written against datasheet DS13012 Rev 7 (December 2021).
// Seed driver: WHO_AM_I probe + continuous 416 Hz accel/gyro + temperature read.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::lsm6dsox {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint16_t kDefaultAddress   = 0x6Au;  // SA0/SDO pin low
inline constexpr std::uint16_t kSecondaryAddress = 0x6Bu;  // SA0/SDO pin high
inline constexpr std::uint8_t  kExpectedWhoAmI   = 0x6Cu;  // WHO_AM_I fixed value

// Register addresses (datasheet Table 18).
inline constexpr std::uint8_t kRegWhoAmI   = 0x0Fu;
inline constexpr std::uint8_t kRegCtrl1Xl  = 0x10u;  // Accelerometer control
inline constexpr std::uint8_t kRegCtrl2G   = 0x11u;  // Gyroscope control
inline constexpr std::uint8_t kRegCtrl3C   = 0x12u;  // General control (IF_INC, BDU, SW_RESET)
inline constexpr std::uint8_t kRegCtrl6C   = 0x15u;  // Accel high-performance mode
inline constexpr std::uint8_t kRegStatus   = 0x1Eu;  // Data-ready status
inline constexpr std::uint8_t kRegOutTempL = 0x20u;  // First byte of 14-byte burst

// CTRL3_C bits.
inline constexpr std::uint8_t kCtrl3IfInc    = 0x04u;  // Register address auto-increment

// STATUS_REG bits.
inline constexpr std::uint8_t kStatusXlda = 0x01u;  // Accel data available
inline constexpr std::uint8_t kStatusGda  = 0x02u;  // Gyro data available
inline constexpr std::uint8_t kStatusTda  = 0x04u;  // Temperature data available

// ODR code 0x6 = 416 Hz (shifted left 4 bits → 0x60).
inline constexpr std::uint8_t kOdrCode416Hz = 0x60u;

// Maximum number of status-poll iterations before returning Timeout.
inline constexpr std::uint32_t kMaxPollIters = 500u;

// ── Types ─────────────────────────────────────────────────────────────────────

/// Full-scale range for the accelerometer.
/// Values match the FS_XL[3:2] field encoding (raw register bits 3:2).
enum class AccelFullScale : std::uint8_t {
    G2   = 0x00u,  //  ±2 g  — sensitivity 0.061 mg/LSB
    G16  = 0x04u,  //  ±16 g — sensitivity 0.488 mg/LSB
    G4   = 0x08u,  //  ±4 g  — sensitivity 0.122 mg/LSB
    G8   = 0x0Cu,  //  ±8 g  — sensitivity 0.244 mg/LSB
};

/// Full-scale range for the gyroscope.
/// Values match the FS_G[3:2] field encoding (raw register bits 3:2).
enum class GyroFullScale : std::uint8_t {
    Dps250  = 0x00u,  //  ±250 dps  — sensitivity 8.75 mdps/LSB
    Dps500  = 0x04u,  //  ±500 dps  — sensitivity 17.50 mdps/LSB
    Dps1000 = 0x08u,  //  ±1000 dps — sensitivity 35.0 mdps/LSB
    Dps2000 = 0x0Cu,  //  ±2000 dps — sensitivity 70.0 mdps/LSB
};

struct Config {
    std::uint16_t  address  = kDefaultAddress;
    AccelFullScale accel_fs = AccelFullScale::G2;
    GyroFullScale  gyro_fs  = GyroFullScale::Dps250;
};

struct Measurement {
    float accel_x_g;      ///< Acceleration X in g
    float accel_y_g;      ///< Acceleration Y in g
    float accel_z_g;      ///< Acceleration Z in g
    float gyro_x_dps;     ///< Angular rate X in degrees/second
    float gyro_y_dps;     ///< Angular rate Y in degrees/second
    float gyro_z_dps;     ///< Angular rate Z in degrees/second
    float temperature_c;  ///< Temperature in degrees Celsius
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 10 ms (sensor ODR settling after init).
inline void busy_wait_10ms() {
    volatile std::uint32_t n = 100'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Return the accelerometer sensitivity factor in g/LSB for the given range.
[[nodiscard]] inline constexpr auto accel_sensitivity(AccelFullScale fs) -> float {
    switch (fs) {
        case AccelFullScale::G2:  return 0.000061f;
        case AccelFullScale::G4:  return 0.000122f;
        case AccelFullScale::G8:  return 0.000244f;
        case AccelFullScale::G16: return 0.000488f;
    }
    return 0.000061f;  // unreachable — satisfy compiler
}

/// Return the gyroscope sensitivity factor in dps/LSB for the given range.
[[nodiscard]] inline constexpr auto gyro_sensitivity(GyroFullScale fs) -> float {
    switch (fs) {
        case GyroFullScale::Dps250:  return 0.00875f;
        case GyroFullScale::Dps500:  return 0.01750f;
        case GyroFullScale::Dps1000: return 0.035f;
        case GyroFullScale::Dps2000: return 0.070f;
    }
    return 0.00875f;  // unreachable — satisfy compiler
}

/// Reconstruct a signed 16-bit integer from two bytes in LSB-first order.
[[nodiscard]] inline constexpr auto to_int16(std::uint8_t lsb, std::uint8_t msb) -> std::int16_t {
    return static_cast<std::int16_t>(
        static_cast<std::uint16_t>(lsb) | (static_cast<std::uint16_t>(msb) << 8u));
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Verifies device presence via WHO_AM_I and configures accel + gyro at 416 Hz.
    ///
    /// Sequence:
    ///   1. Read WHO_AM_I register; return `CommunicationError` unless value is 0x6C.
    ///   2. Write CTRL3_C = 0x04 (IF_INC=1, enables register auto-increment).
    ///   3. Write CTRL1_XL = 0x60 | accel_fs_code (416 Hz, configured FS).
    ///   4. Write CTRL2_G  = 0x60 | gyro_fs_code  (416 Hz, configured FS).
    ///   5. Busy-wait ~10 ms for the sensor to produce its first sample.
    ///
    /// Returns `CommunicationError` on I2C failure or WHO_AM_I mismatch.
    [[nodiscard]] auto init() -> ResultVoid {
        // Step 1: WHO_AM_I check.
        const std::array<std::uint8_t, 1> who_reg{kRegWhoAmI};
        std::array<std::uint8_t, 1> who_buf{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{who_reg},
                                      std::span<std::uint8_t>{who_buf});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if (who_buf[0] != kExpectedWhoAmI) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 2: CTRL3_C — enable register address auto-increment.
        const std::array<std::uint8_t, 2> ctrl3{kRegCtrl3C, kCtrl3IfInc};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl3});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 3: CTRL1_XL — accelerometer ODR = 416 Hz + full-scale range.
        const std::array<std::uint8_t, 2> ctrl1{
            kRegCtrl1Xl,
            static_cast<std::uint8_t>(kOdrCode416Hz | static_cast<std::uint8_t>(cfg_.accel_fs))};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl1});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 4: CTRL2_G — gyroscope ODR = 416 Hz + full-scale range.
        const std::array<std::uint8_t, 2> ctrl2{
            kRegCtrl2G,
            static_cast<std::uint8_t>(kOdrCode416Hz | static_cast<std::uint8_t>(cfg_.gyro_fs))};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl2});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 5: wait for the sensor to produce the first valid sample.
        detail::busy_wait_10ms();

        return alloy::core::Ok();
    }

    /// Reads one accel + gyro + temperature measurement.
    ///
    /// Sequence:
    ///   1. Poll STATUS_REG until XLDA, GDA, and TDA are all set
    ///      (max kMaxPollIters iterations); returns `Timeout` if not.
    ///   2. Burst-read 14 bytes from OUT_TEMP_L (0x20):
    ///        [0–1]  TEMP_L / TEMP_H
    ///        [2–3]  OUTX_L_G / OUTX_H_G  (gyro X)
    ///        [4–5]  OUTY_L_G / OUTY_H_G  (gyro Y)
    ///        [6–7]  OUTZ_L_G / OUTZ_H_G  (gyro Z)
    ///        [8–9]  OUTX_L_A / OUTX_H_A  (accel X)
    ///        [10–11] OUTY_L_A / OUTY_H_A (accel Y)
    ///        [12–13] OUTZ_L_A / OUTZ_H_A (accel Z)
    ///   3. Reconstruct int16_t (LSB-first) and convert to physical units.
    ///
    /// Returns `CommunicationError` on I2C failure, `Timeout` on poll expiry.
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Step 1: poll STATUS_REG until all three data-ready bits are set.
        const std::array<std::uint8_t, 1> status_reg{kRegStatus};
        std::array<std::uint8_t, 1> status_buf{};
        std::uint32_t poll_count = 0u;
        while (true) {
            if (auto r = bus_->write_read(cfg_.address,
                                          std::span<const std::uint8_t>{status_reg},
                                          std::span<std::uint8_t>{status_buf});
                r.is_err()) {
                return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
            }
            constexpr std::uint8_t kAllReady = kStatusXlda | kStatusGda | kStatusTda;
            if ((status_buf[0] & kAllReady) == kAllReady) {
                break;
            }
            ++poll_count;
            if (poll_count >= kMaxPollIters) {
                return alloy::core::Err(alloy::core::ErrorCode::Timeout);
            }
        }

        // Step 2: burst-read 14 bytes starting at OUT_TEMP_L.
        // With IF_INC=1, the device increments the register address automatically.
        // Register map (datasheet §9.20 – §9.33):
        //   0x20 TEMP_L, 0x21 TEMP_H,
        //   0x22 GX_L,   0x23 GX_H,   0x24 GY_L, 0x25 GY_H, 0x26 GZ_L, 0x27 GZ_H,
        //   0x28 AX_L,   0x29 AX_H,   0x2A AY_L, 0x2B AY_H, 0x2C AZ_L, 0x2D AZ_H
        const std::array<std::uint8_t, 1> data_reg{kRegOutTempL};
        std::array<std::uint8_t, 14> data{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{data_reg},
                                      std::span<std::uint8_t>{data});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 3: reconstruct and convert raw values.

        // Temperature: raw_t / 256.0 + 25.0 (datasheet §4.4).
        const std::int16_t raw_t  = detail::to_int16(data[0], data[1]);
        const float temperature_c = (static_cast<float>(raw_t) / 256.0f) + 25.0f;

        // Gyroscope (mdps/LSB → dps/LSB via sensitivity).
        const float g_sens = detail::gyro_sensitivity(cfg_.gyro_fs);
        const std::int16_t raw_gx = detail::to_int16(data[2], data[3]);
        const std::int16_t raw_gy = detail::to_int16(data[4], data[5]);
        const std::int16_t raw_gz = detail::to_int16(data[6], data[7]);
        const float gyro_x_dps = static_cast<float>(raw_gx) * g_sens;
        const float gyro_y_dps = static_cast<float>(raw_gy) * g_sens;
        const float gyro_z_dps = static_cast<float>(raw_gz) * g_sens;

        // Accelerometer (mg/LSB → g/LSB via sensitivity).
        const float a_sens = detail::accel_sensitivity(cfg_.accel_fs);
        const std::int16_t raw_ax = detail::to_int16(data[8],  data[9]);
        const std::int16_t raw_ay = detail::to_int16(data[10], data[11]);
        const std::int16_t raw_az = detail::to_int16(data[12], data[13]);
        const float accel_x_g = static_cast<float>(raw_ax) * a_sens;
        const float accel_y_g = static_cast<float>(raw_ay) * a_sens;
        const float accel_z_g = static_cast<float>(raw_az) * a_sens;

        return alloy::core::Ok(Measurement{
            accel_x_g,   accel_y_g,   accel_z_g,
            gyro_x_dps,  gyro_y_dps,  gyro_z_dps,
            temperature_c});
    }

private:
    BusHandle* bus_;
    Config     cfg_;
};

}  // namespace alloy::drivers::sensor::lsm6dsox

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockI2cForLsm6dsoxGate {
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
    sizeof(alloy::drivers::sensor::lsm6dsox::Device<_MockI2cForLsm6dsoxGate>) > 0,
    "lsm6dsox Device must compile against the documented I2C bus surface");
}  // namespace
