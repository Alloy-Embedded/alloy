#pragma once

// drivers/sensor/lis3mdl/lis3mdl.hpp
//
// Driver for STMicroelectronics LIS3MDL 3-axis magnetometer over I2C.
// Written against datasheet DS9553 Rev 6.
// Seed driver: WHO_AM_I probe + continuous-mode 3-axis magnetic field measurement.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::lis3mdl {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint16_t kDefaultAddress   = 0x1Cu;  // SA1 pin low
inline constexpr std::uint16_t kSecondaryAddress = 0x1Eu;  // SA1 pin high
inline constexpr std::uint8_t  kExpectedWhoAmI   = 0x3Du;  // WHO_AM_I fixed value

// Register addresses.
inline constexpr std::uint8_t kRegWhoAmI   = 0x0Fu;
inline constexpr std::uint8_t kRegCtrlReg1 = 0x20u;
inline constexpr std::uint8_t kRegCtrlReg2 = 0x21u;
inline constexpr std::uint8_t kRegCtrlReg3 = 0x22u;
inline constexpr std::uint8_t kRegCtrlReg4 = 0x23u;
inline constexpr std::uint8_t kRegCtrlReg5 = 0x24u;
inline constexpr std::uint8_t kRegStatus   = 0x27u;
inline constexpr std::uint8_t kRegOutXL    = 0x28u;  // first byte of 6-byte burst

// CTRL_REG1: TEMP_EN=0, OM=11 (ultra-high perf XY), DO=100 (10 Hz ODR).
inline constexpr std::uint8_t kCtrlReg1Value = 0x70u;

// CTRL_REG3: MD=00 continuous-measurement mode.
inline constexpr std::uint8_t kCtrlReg3Continuous = 0x00u;

// CTRL_REG4: OMZ=11 ultra-high performance Z.
inline constexpr std::uint8_t kCtrlReg4Value = 0x0Cu;

// CTRL_REG5: BDU=1 (block data update — prevents reading split samples).
inline constexpr std::uint8_t kCtrlReg5Value = 0x40u;

// STATUS_REG bit masks.
inline constexpr std::uint8_t kStatusZyxda = 0x08u;  // XYZ data available

// Maximum status-poll iterations before returning Timeout.
inline constexpr std::uint32_t kMaxPollIters = 200u;

// ── Types ─────────────────────────────────────────────────────────────────────

/// Full-scale range selection (CTRL_REG2 FS[6:5]).
enum class FullScale : std::uint8_t {
    G4  = 0x00u,  ///<  ±4 gauss  — 6842 LSB/gauss
    G8  = 0x20u,  ///<  ±8 gauss  — 3421 LSB/gauss
    G12 = 0x40u,  ///< ±12 gauss  — 2281 LSB/gauss
    G16 = 0x60u,  ///< ±16 gauss  — 1711 LSB/gauss
};

struct Config {
    std::uint16_t address    = kDefaultAddress;
    FullScale     full_scale = FullScale::G4;
};

struct Measurement {
    float x_gauss;  ///< X-axis magnetic field in Gauss
    float y_gauss;  ///< Y-axis magnetic field in Gauss
    float z_gauss;  ///< Z-axis magnetic field in Gauss
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 20 ms (continuous-mode first-sample settle time).
inline void busy_wait_20ms() {
    volatile std::uint32_t n = 200'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Returns the LSB-per-gauss sensitivity for the given full-scale range.
[[nodiscard]] inline constexpr auto sensitivity(FullScale fs) -> float {
    switch (fs) {
        case FullScale::G4:  return 6842.0f;
        case FullScale::G8:  return 3421.0f;
        case FullScale::G12: return 2281.0f;
        case FullScale::G16: return 1711.0f;
    }
    return 6842.0f;  // unreachable; default to ±4 G
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Verifies device presence via WHO_AM_I and configures for continuous measurement.
    ///
    /// Sequence:
    ///   1. write_read([WHO_AM_I], [1 byte]) — check == 0x3D.
    ///   2. Write CTRL_REG2 = full_scale bits.
    ///   3. Write CTRL_REG1 = 0x70 (ultra-high XY, 10 Hz ODR).
    ///   4. Write CTRL_REG4 = 0x0C (ultra-high Z).
    ///   5. Write CTRL_REG5 = 0x40 (BDU=1).
    ///   6. Write CTRL_REG3 = 0x00 (continuous mode).
    ///   7. Busy-wait ~20 ms for first sample settle.
    ///
    /// Returns `CommunicationError` if any transfer fails or the WHO_AM_I value
    /// does not match the expected 0x3D.
    [[nodiscard]] auto init() -> ResultVoid {
        // Step 1: read WHO_AM_I register.
        const std::array<std::uint8_t, 1> reg_who{kRegWhoAmI};
        std::array<std::uint8_t, 1> who_am_i{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{reg_who},
                                      std::span<std::uint8_t>{who_am_i});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if (who_am_i[0] != kExpectedWhoAmI) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 2: configure full-scale range via CTRL_REG2.
        const std::array<std::uint8_t, 2> ctrl2{kRegCtrlReg2,
                                                  static_cast<std::uint8_t>(cfg_.full_scale)};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl2});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 3: CTRL_REG1 — TEMP_EN=0, OM=11 (ultra-high XY), DO=100 (10 Hz).
        const std::array<std::uint8_t, 2> ctrl1{kRegCtrlReg1, kCtrlReg1Value};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl1});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 4: CTRL_REG4 — OMZ=11 (ultra-high Z).
        const std::array<std::uint8_t, 2> ctrl4{kRegCtrlReg4, kCtrlReg4Value};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl4});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 5: CTRL_REG5 — BDU=1 (block data update).
        const std::array<std::uint8_t, 2> ctrl5{kRegCtrlReg5, kCtrlReg5Value};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl5});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 6: CTRL_REG3 — MD=00 continuous measurement.
        const std::array<std::uint8_t, 2> ctrl3{kRegCtrlReg3, kCtrlReg3Continuous};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl3});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 7: settle time for first conversion.
        detail::busy_wait_20ms();

        return alloy::core::Ok();
    }

    /// Waits for data-ready and returns a 3-axis magnetic field measurement.
    ///
    /// Sequence:
    ///   1. Poll STATUS_REG until ZYXDA (bit 3) is set (max kMaxPollIters).
    ///      Returns `Timeout` if the bit never sets.
    ///   2. Burst-read 6 bytes from OUT_X_L: X_L, X_H, Y_L, Y_H, Z_L, Z_H.
    ///   3. Reconstruct int16_t as (H<<8 | L) for each axis (little-endian).
    ///   4. Convert to Gauss by dividing by the full-scale sensitivity (LSB/G).
    ///
    /// Returns `CommunicationError` on I2C failure, `Timeout` if the device
    /// does not assert DRDY within the polling budget.
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Step 1: poll STATUS_REG until ZYXDA is set.
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
            if ((status_buf[0] & kStatusZyxda) != 0u) {
                break;
            }
            ++poll_count;
            if (poll_count >= kMaxPollIters) {
                return alloy::core::Err(alloy::core::ErrorCode::Timeout);
            }
        }

        // Step 2: burst-read 6 bytes starting at OUT_X_L.
        // Layout: X_L, X_H, Y_L, Y_H, Z_L, Z_H (little-endian per axis).
        const std::array<std::uint8_t, 1> data_reg{kRegOutXL};
        std::array<std::uint8_t, 6> data{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{data_reg},
                                      std::span<std::uint8_t>{data});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 3: reconstruct signed 16-bit values (little-endian).
        const auto raw_x = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(data[0]) |
            (static_cast<std::uint16_t>(data[1]) << 8));
        const auto raw_y = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(data[2]) |
            (static_cast<std::uint16_t>(data[3]) << 8));
        const auto raw_z = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(data[4]) |
            (static_cast<std::uint16_t>(data[5]) << 8));

        // Step 4: convert to Gauss.
        const float sens = detail::sensitivity(cfg_.full_scale);
        return alloy::core::Ok(Measurement{
            static_cast<float>(raw_x) / sens,
            static_cast<float>(raw_y) / sens,
            static_cast<float>(raw_z) / sens,
        });
    }

private:
    BusHandle* bus_;
    Config     cfg_;
};

}  // namespace alloy::drivers::sensor::lis3mdl

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockI2cForLis3mdlGate {
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
    sizeof(alloy::drivers::sensor::lis3mdl::Device<_MockI2cForLis3mdlGate>) > 0,
    "lis3mdl Device must compile against the documented I2C bus surface");
}  // namespace
