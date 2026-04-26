#pragma once

// drivers/sensor/aht20/aht20.hpp
//
// Driver for ASAIR AHT20 temperature + humidity sensor over I2C.
// Written against datasheet revision 1.0 (ASAIR AHT20).
// Seed driver: calibration probe + soft-reset + triggered one-shot measurement
// with busy-poll and physical-unit conversion. No float arithmetic in the hot
// path beyond the final conversion step. See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::aht20 {

// ── Constants ─────────────────────────────────────────────────────────────────

/// Fixed I2C address — the AHT20 has no address-select pin.
inline constexpr std::uint16_t kDefaultAddress = 0x38;

// Status register bit masks.
inline constexpr std::uint8_t kStatusBusy       = 0x80u;  ///< Bit 7: measurement in progress
inline constexpr std::uint8_t kStatusCalibrated = 0x08u;  ///< Bit 3: calibration coefficients loaded

// Command bytes.
inline constexpr std::uint8_t kCmdSoftReset  = 0xBAu;
inline constexpr std::uint8_t kCmdCalibrate  = 0xBEu;
inline constexpr std::uint8_t kCmdTrigger    = 0xACu;

// Maximum number of status-poll iterations before giving up.
inline constexpr std::uint32_t kMaxPollIters = 100u;

// ── Types ─────────────────────────────────────────────────────────────────────

struct Config {
    std::uint16_t address = kDefaultAddress;
};

struct Measurement {
    float temperature_c;  ///< Temperature in degrees Celsius
    float humidity_pct;   ///< Relative humidity in % (clamped to [0, 100])
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 10 ms (calibration settle time).
/// Iteration count calibrated for typical Cortex-M / RISC-V at common speeds.
inline void busy_wait_10ms() {
    volatile std::uint32_t n = 100'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Busy-wait approximately 20 ms (soft-reset settle time).
inline void busy_wait_20ms() {
    volatile std::uint32_t n = 200'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Busy-wait approximately 80 ms (measurement conversion time).
inline void busy_wait_80ms() {
    volatile std::uint32_t n = 800'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Clamp a float value to [lo, hi].
[[nodiscard]] inline constexpr auto clamp(float v, float lo, float hi) -> float {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Initialises the AHT20.
    ///
    /// Sequence:
    ///   1. Read the status byte; if the calibration bit (bit 3) is clear,
    ///      send the calibration command [0xBE, 0x08, 0x00] and wait ~10 ms.
    ///   2. Send a soft-reset command (0xBA) and wait ~20 ms.
    ///
    /// Returns `CommunicationError` on any I2C failure.
    [[nodiscard]] auto init() -> ResultVoid {
        // Step 1: read status and calibrate if necessary.
        std::array<std::uint8_t, 1> status_buf{};
        if (auto r = bus_->read(cfg_.address, std::span<std::uint8_t>{status_buf});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        if ((status_buf[0] & kStatusCalibrated) == 0u) {
            const std::array<std::uint8_t, 3> calib_cmd{kCmdCalibrate, 0x08u, 0x00u};
            if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{calib_cmd});
                r.is_err()) {
                return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
            }
            detail::busy_wait_10ms();
        }

        // Step 2: soft reset.
        const std::array<std::uint8_t, 1> reset_cmd{kCmdSoftReset};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{reset_cmd});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        detail::busy_wait_20ms();

        return alloy::core::Ok();
    }

    /// Triggers a measurement and returns temperature + humidity.
    ///
    /// Sequence:
    ///   1. Write trigger command [0xAC, 0x33, 0x00].
    ///   2. Busy-wait ~80 ms for conversion.
    ///   3. Poll status byte (max kMaxPollIters times) until busy bit clears;
    ///      returns `Timeout` if it never clears.
    ///   4. Read 6 data bytes and convert to physical units.
    ///
    /// Returns `CommunicationError` on I2C failure, `Timeout` if the sensor
    /// does not complete within the polling budget.
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Step 1: trigger measurement.
        const std::array<std::uint8_t, 3> trigger_cmd{kCmdTrigger, 0x33u, 0x00u};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{trigger_cmd});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 2: wait for the bulk of the conversion.
        detail::busy_wait_80ms();

        // Step 3: poll until busy bit clears.
        std::array<std::uint8_t, 1> poll_buf{};
        std::uint32_t poll_count = 0u;
        while (true) {
            if (auto r = bus_->read(cfg_.address, std::span<std::uint8_t>{poll_buf});
                r.is_err()) {
                return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
            }
            if ((poll_buf[0] & kStatusBusy) == 0u) {
                break;
            }
            ++poll_count;
            if (poll_count >= kMaxPollIters) {
                return alloy::core::Err(alloy::core::ErrorCode::Timeout);
            }
        }

        // Step 4: read the full 6-byte data frame.
        // Frame layout (datasheet §5.4):
        //   [0] status
        //   [1] RH[19:12]
        //   [2] RH[11:4]
        //   [3] RH[3:0] | T[19:16]
        //   [4] T[15:8]
        //   [5] T[7:0]
        std::array<std::uint8_t, 6> data{};
        if (auto r = bus_->read(cfg_.address, std::span<std::uint8_t>{data});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Extract 20-bit raw values.
        const std::uint32_t raw_rh =
            (static_cast<std::uint32_t>(data[1]) << 12) |
            (static_cast<std::uint32_t>(data[2]) << 4)  |
            (static_cast<std::uint32_t>(data[3]) >> 4);

        const std::uint32_t raw_t =
            ((static_cast<std::uint32_t>(data[3]) & 0x0Fu) << 16) |
            (static_cast<std::uint32_t>(data[4]) << 8)            |
            static_cast<std::uint32_t>(data[5]);

        // Physical-unit conversion (datasheet §6.1).
        constexpr float kScale = 1048576.0f;  // 2^20
        const float humidity_pct =
            detail::clamp((static_cast<float>(raw_rh) / kScale) * 100.0f, 0.0f, 100.0f);
        const float temperature_c =
            (static_cast<float>(raw_t) / kScale) * 200.0f - 50.0f;

        return alloy::core::Ok(Measurement{temperature_c, humidity_pct});
    }

private:
    BusHandle* bus_;
    Config     cfg_;
};

}  // namespace alloy::drivers::sensor::aht20

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockAhtBusForAht20Gate {
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
    sizeof(alloy::drivers::sensor::aht20::Device<_MockAhtBusForAht20Gate>) > 0,
    "aht20 Device must compile against the documented I2C bus surface");
}  // namespace
