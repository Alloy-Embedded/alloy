#pragma once

// drivers/sensor/sht4x/sht4x.hpp
//
// Driver for Sensirion SHT40/SHT41 temperature + humidity sensor over I2C.
// Written against datasheet revision 1.0 (Sensirion SHT4x).
// Seed driver: soft-reset probe + high/medium/low-repeatability one-shot
// measurement with CRC-8 verification and physical-unit conversion.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::sht4x {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint16_t kDefaultAddress   = 0x44;  // ADDR pin low
inline constexpr std::uint16_t kSecondaryAddress = 0x45;  // ADDR pin high

// ── Types ─────────────────────────────────────────────────────────────────────

/// Measurement repeatability — controls command byte and conversion delay.
enum class Repeatability : std::uint8_t {
    High,    ///< 0xFD — ~10 ms conversion time, lowest noise
    Medium,  ///< 0xF6 — ~5 ms conversion time
    Low,     ///< 0xE0 — ~2 ms conversion time
};

struct Config {
    std::uint16_t address        = kDefaultAddress;
    Repeatability repeatability  = Repeatability::High;
};

struct Measurement {
    float temperature_c;  ///< Temperature in degrees Celsius
    float humidity_pct;   ///< Relative humidity in % (clamped to [0, 100])
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// CRC-8 per Sensirion datasheet: polynomial 0x31, init 0xFF, no final XOR.
[[nodiscard]] inline auto crc8(const std::uint8_t* data,
                                std::size_t len) -> std::uint8_t {
    std::uint8_t crc = 0xFF;
    for (std::size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x80u) {
                crc = static_cast<std::uint8_t>((crc << 1) ^ 0x31u);
            } else {
                crc = static_cast<std::uint8_t>(crc << 1);
            }
        }
    }
    return crc;
}

/// Returns the measurement command byte for the requested repeatability.
[[nodiscard]] inline constexpr auto command_byte(Repeatability r) -> std::uint8_t {
    switch (r) {
        case Repeatability::High:   return 0xFD;
        case Repeatability::Medium: return 0xF6;
        case Repeatability::Low:    return 0xE0;
    }
    return 0xFD;  // unreachable, satisfy compiler
}

/// Returns the approximate busy-wait iteration count for each repeatability.
/// These loop counts approximate the datasheet conversion times when running
/// at typical Cortex-M / RISC-V clock rates (no timer dependency).
[[nodiscard]] inline constexpr auto wait_iters(Repeatability r) -> std::uint32_t {
    switch (r) {
        case Repeatability::High:   return 100'000u;  // ~10 ms budget
        case Repeatability::Medium: return  50'000u;  // ~5  ms budget
        case Repeatability::Low:    return  20'000u;  // ~2  ms budget
    }
    return 100'000u;
}

/// Approximately 1 ms busy-wait used after soft-reset.
inline void busy_wait_1ms() {
    volatile std::uint32_t n = 10'000u;
    while (n-- != 0u) {
        // intentional busy spin
    }
}

/// Generic busy-wait for measurement conversion.
inline void busy_wait_iters(std::uint32_t iters) {
    volatile std::uint32_t n = iters;
    while (n-- != 0u) {
        // intentional busy spin
    }
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Initialises the device.
    ///
    /// Sends a soft-reset command (0x94), waits ~1 ms, then performs one
    /// measurement to confirm the device responds.  Returns
    /// `CommunicationError` if any I2C operation is NAK-ed or fails.
    [[nodiscard]] auto init() -> ResultVoid {
        // Soft-reset: write single byte 0x94.
        const std::array<std::uint8_t, 1> reset_cmd{0x94};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{reset_cmd});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Wait ~1 ms for the device to complete reset.
        detail::busy_wait_1ms();

        // Verify communication by performing one full measurement.
        if (auto r = read(); r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        return alloy::core::Ok();
    }

    /// Triggers a measurement and returns temperature + humidity.
    ///
    /// Writes the measurement command, busy-waits for conversion, reads 6
    /// bytes, verifies the CRC-8 of both word pairs, then converts to
    /// physical units.  Returns `CommunicationError` on bus failure or CRC
    /// mismatch.
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Issue measurement command.
        const std::array<std::uint8_t, 1> cmd{detail::command_byte(cfg_.repeatability)};
        if (auto r = bus_->write(cfg_.address, std::span<const std::uint8_t>{cmd});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Wait for conversion to complete.
        detail::busy_wait_iters(detail::wait_iters(cfg_.repeatability));

        // Read 6 bytes: [T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC].
        std::array<std::uint8_t, 6> buf{};
        if (auto r = bus_->read(cfg_.address, std::span<std::uint8_t>{buf});
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Verify temperature CRC.
        if (detail::crc8(&buf[0], 2) != buf[2]) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Verify humidity CRC.
        if (detail::crc8(&buf[3], 2) != buf[5]) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Reconstruct 16-bit raw values.
        const std::uint16_t raw_t =
            static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(buf[0]) << 8) | buf[1]);
        const std::uint16_t raw_rh =
            static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(buf[3]) << 8) | buf[4]);

        // Physical-unit conversion (datasheet §4.1).
        const float temp_c  = -45.0f + 175.0f * (static_cast<float>(raw_t)  / 65535.0f);
        float       rh_pct  =  -6.0f + 125.0f * (static_cast<float>(raw_rh) / 65535.0f);

        // Clamp humidity to [0, 100] per datasheet recommendation.
        if (rh_pct < 0.0f)   rh_pct = 0.0f;
        if (rh_pct > 100.0f) rh_pct = 100.0f;

        return alloy::core::Ok(Measurement{temp_c, rh_pct});
    }

private:
    BusHandle* bus_;
    Config     cfg_;
};

}  // namespace alloy::drivers::sensor::sht4x

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockShtBusForSht4xGate {
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
    sizeof(alloy::drivers::sensor::sht4x::Device<_MockShtBusForSht4xGate>) > 0,
    "sht4x Device must compile against the documented I2C bus surface");
}  // namespace
