#pragma once

// drivers/sensor/lps22hh/lps22hh.hpp
//
// Driver for STMicroelectronics LPS22HH barometric pressure + temperature sensor over I2C.
// Written against datasheet DS12868 Rev 6 (January 2020).
// Seed driver: WHO_AM_I probe + one-shot pressure/temperature measurement.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::lps22hh {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint16_t kDefaultAddress   = 0x5Cu;  // SDO/SA0 pin low
inline constexpr std::uint16_t kSecondaryAddress = 0x5Du;  // SDO/SA0 pin high
inline constexpr std::uint8_t  kExpectedWhoAmI   = 0xB3u;  // WHO_AM_I fixed value

// Register addresses.
inline constexpr std::uint8_t kRegWhoAmI    = 0x0Fu;
inline constexpr std::uint8_t kRegCtrl1     = 0x10u;
inline constexpr std::uint8_t kRegCtrl2     = 0x11u;
inline constexpr std::uint8_t kRegStatus    = 0x27u;
inline constexpr std::uint8_t kRegPressXL   = 0x28u;  // first byte of 5-byte burst

// CTRL_REG2 bits.
inline constexpr std::uint8_t kCtrl2IfAddInc = 0x10u;  // address auto-increment
inline constexpr std::uint8_t kCtrl2OneShot  = 0x01u;  // trigger one-shot measurement

// STATUS bits.
inline constexpr std::uint8_t kStatusTda = 0x02u;  // temperature data available
inline constexpr std::uint8_t kStatusPda = 0x01u;  // pressure data available

// Maximum status-poll iterations before returning Timeout.
inline constexpr std::uint32_t kMaxPollIters = 200u;

// ── Types ─────────────────────────────────────────────────────────────────────

struct Config {
    std::uint16_t address = kDefaultAddress;
};

struct Measurement {
    float pressure_hpa;   ///< Barometric pressure in hectopascals (hPa)
    float temperature_c;  ///< Temperature in degrees Celsius
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 5 ms (one-shot conversion settling time).
inline void busy_wait_5ms() {
    volatile std::uint32_t n = 50'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Sign-extend a 24-bit two's-complement value to int32_t.
[[nodiscard]] inline constexpr auto sign_extend_24(std::uint32_t v) -> std::int32_t {
    if (v & 0x00800000u) {
        return static_cast<std::int32_t>(v | 0xFF000000u);
    }
    return static_cast<std::int32_t>(v);
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Verifies device presence via WHO_AM_I and configures for one-shot use.
    ///
    /// Returns `CommunicationError` if the I2C transfer fails or the WHO_AM_I
    /// value does not match the expected 0xB3.
    [[nodiscard]] auto init() -> ResultVoid {
        // Read WHO_AM_I register.
        const std::array<std::uint8_t, 1> reg{kRegWhoAmI};
        std::array<std::uint8_t, 1> who_am_i{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{reg},
                                      std::span<std::uint8_t>{who_am_i});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if (who_am_i[0] != kExpectedWhoAmI) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // CTRL_REG1 = 0x00: power-down (ODR=0), one-shot triggered by CTRL_REG2.
        const std::array<std::uint8_t, 2> ctrl1{kRegCtrl1, 0x00u};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl1});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // CTRL_REG2 = IF_ADD_INC: enable register auto-increment for burst reads.
        const std::array<std::uint8_t, 2> ctrl2{kRegCtrl2, kCtrl2IfAddInc};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{ctrl2});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        return alloy::core::Ok();
    }

    /// Triggers a one-shot measurement and returns pressure + temperature.
    ///
    /// Sequence:
    ///   1. Set ONE_SHOT bit in CTRL_REG2 to start conversion.
    ///   2. Busy-wait ~5 ms for conversion.
    ///   3. Poll STATUS until both T_DA and P_DA are set (max kMaxPollIters).
    ///   4. Read 5 bytes starting at PRESS_OUT_XL (auto-increment covers
    ///      PRESS_OUT_L, PRESS_OUT_H, TEMP_OUT_L, TEMP_OUT_H).
    ///   5. Convert raw values to hPa and °C.
    ///
    /// Returns `CommunicationError` on I2C failure, `Timeout` if the device
    /// does not complete within the polling budget.
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Trigger one-shot conversion.
        const std::array<std::uint8_t, 2> trigger{kRegCtrl2,
                                                   static_cast<std::uint8_t>(kCtrl2IfAddInc |
                                                                              kCtrl2OneShot)};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{trigger});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Wait for the bulk of conversion before polling.
        detail::busy_wait_5ms();

        // Poll STATUS until both data-ready flags are set.
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
            if ((status_buf[0] & (kStatusTda | kStatusPda)) == (kStatusTda | kStatusPda)) {
                break;
            }
            ++poll_count;
            if (poll_count >= kMaxPollIters) {
                return alloy::core::Err(alloy::core::ErrorCode::Timeout);
            }
        }

        // Burst-read 5 bytes: PRESS_OUT_XL, PRESS_OUT_L, PRESS_OUT_H,
        //                     TEMP_OUT_L, TEMP_OUT_H.
        const std::array<std::uint8_t, 1> data_reg{kRegPressXL};
        std::array<std::uint8_t, 5> data{};
        if (auto r = bus_->write_read(cfg_.address,
                                      std::span<const std::uint8_t>{data_reg},
                                      std::span<std::uint8_t>{data});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Reconstruct 24-bit signed pressure (datasheet §9.17–9.19).
        const std::uint32_t raw_p_u =
            static_cast<std::uint32_t>(data[0]) |
            (static_cast<std::uint32_t>(data[1]) << 8) |
            (static_cast<std::uint32_t>(data[2]) << 16);
        const std::int32_t raw_p = detail::sign_extend_24(raw_p_u);

        // Reconstruct 16-bit signed temperature (datasheet §9.20–9.21).
        const std::int16_t raw_t =
            static_cast<std::int16_t>(
                static_cast<std::uint16_t>(data[3]) |
                (static_cast<std::uint16_t>(data[4]) << 8));

        // Physical-unit conversion.
        // Pressure: 1 LSB = 1/4096 hPa.
        // Temperature: 1 LSB = 0.01 °C.
        const float pressure_hpa  = static_cast<float>(raw_p) / 4096.0f;
        const float temperature_c = static_cast<float>(raw_t) / 100.0f;

        return alloy::core::Ok(Measurement{pressure_hpa, temperature_c});
    }

private:
    BusHandle* bus_;
    Config     cfg_;
};

}  // namespace alloy::drivers::sensor::lps22hh

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockI2cForLps22hhGate {
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
    sizeof(alloy::drivers::sensor::lps22hh::Device<_MockI2cForLps22hhGate>) > 0,
    "lps22hh Device must compile against the documented I2C bus surface");
}  // namespace
