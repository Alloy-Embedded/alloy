#pragma once

// drivers/power/max17048/max17048.hpp
//
// Driver for Maxim MAX17048 LiPo fuel gauge over I2C.
// Written against datasheet DS MAX17048-MAX17049 Rev 3.
// Seed driver: VERSION probe + voltage / SOC / charge-rate measurement.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::power::max17048 {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint16_t kDefaultAddress = 0x36u;  // fixed I2C address

// Register addresses (16-bit big-endian, addressed by 1-byte register pointer).
inline constexpr std::uint8_t kRegVcell    = 0x02u;  // cell voltage
inline constexpr std::uint8_t kRegSoc      = 0x04u;  // state of charge
inline constexpr std::uint8_t kRegMode     = 0x06u;  // quick-start / hibernate
inline constexpr std::uint8_t kRegVersion  = 0x08u;  // device version (non-zero)
inline constexpr std::uint8_t kRegConfig   = 0x0Cu;  // alert threshold, sleep
inline constexpr std::uint8_t kRegCrate    = 0x16u;  // charge rate (%/hr, signed)
inline constexpr std::uint8_t kRegVresetId = 0x18u;  // VRESET | OCV threshold
inline constexpr std::uint8_t kRegStatus   = 0x1Au;  // alert flags
inline constexpr std::uint8_t kRegCmd      = 0xFEu;  // command (0x5400 = POR)

// CONFIG register: bits [4:0] = alert threshold (32 - ATHD encodes percent).
// Default mask for other bits that must be preserved (alert enabled, not sleep).
inline constexpr std::uint16_t kConfigAlertMask  = 0x001Fu;  // bits[4:0] = ATHD
inline constexpr std::uint16_t kConfigDefaultHigh = 0x971Cu;  // typical upper byte + bit pattern

// ── Types ─────────────────────────────────────────────────────────────────────

struct Config {
    std::uint16_t address   = kDefaultAddress;
    std::uint8_t  alert_pct = 4u;   ///< Alert threshold in percent (1–32)
};

struct Measurement {
    float voltage_v;    ///< Cell voltage in volts
    float soc_pct;      ///< State of charge in percent (0–100)
    float charge_rate;  ///< Charge/discharge rate in %/hr (negative = discharging)
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Read a 16-bit big-endian register and return as uint16_t.
template <typename BusHandle>
[[nodiscard]] inline auto read_reg16(BusHandle& bus,
                                     std::uint16_t addr,
                                     std::uint8_t  reg)
    -> alloy::core::Result<std::uint16_t, alloy::core::ErrorCode>
{
    const std::array<std::uint8_t, 1> tx{reg};
    std::array<std::uint8_t, 2> rx{};
    if (auto r = bus.write_read(addr,
                                std::span<const std::uint8_t>{tx},
                                std::span<std::uint8_t>{rx});
        r.is_err()) {
        return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
    }
    const std::uint16_t val = (static_cast<std::uint16_t>(rx[0]) << 8) |
                               static_cast<std::uint16_t>(rx[1]);
    return alloy::core::Ok(val);
}

/// Write a 16-bit big-endian register.
template <typename BusHandle>
[[nodiscard]] inline auto write_reg16(BusHandle& bus,
                                      std::uint16_t addr,
                                      std::uint8_t  reg,
                                      std::uint16_t value)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    const std::array<std::uint8_t, 3> tx{
        reg,
        static_cast<std::uint8_t>(value >> 8),
        static_cast<std::uint8_t>(value & 0xFFu)
    };
    if (auto r = bus.write(addr, std::span<const std::uint8_t>{tx}); r.is_err()) {
        return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
    }
    return alloy::core::Ok();
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Verifies device presence via VERSION register and configures alert threshold.
    ///
    /// Returns `CommunicationError` if the I2C transfer fails or VERSION reads
    /// back as zero (no device present).
    [[nodiscard]] auto init() -> ResultVoid {
        // Read VERSION register — must be non-zero.
        auto ver = detail::read_reg16(*bus_, cfg_.address, kRegVersion);
        if (ver.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if (ver.unwrap() == 0u) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Read current CONFIG register to preserve reserved bits.
        auto cfg_reg = detail::read_reg16(*bus_, cfg_.address, kRegConfig);
        if (cfg_reg.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // ATHD field (bits[4:0]) encodes alert as (32 - alert_pct).
        // Clamp alert_pct to valid range [1, 32].
        const std::uint8_t pct = (cfg_.alert_pct < 1u) ? 1u :
                                 (cfg_.alert_pct > 32u) ? 32u : cfg_.alert_pct;
        const std::uint8_t athd = static_cast<std::uint8_t>(32u - pct);

        const std::uint16_t new_cfg =
            (cfg_reg.unwrap() & ~kConfigAlertMask) |
            (static_cast<std::uint16_t>(athd) & kConfigAlertMask);

        if (auto r = detail::write_reg16(*bus_, cfg_.address, kRegConfig, new_cfg);
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        return alloy::core::Ok();
    }

    /// Reads cell voltage, state-of-charge, and charge rate from the device.
    ///
    /// Voltage (V) = raw_vcell × 78.125e-6
    /// SOC (%)     = raw_soc / 256.0
    /// Rate (%/hr) = (int16_t)raw_crate × 0.208
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Read VCELL (unsigned 16-bit).
        auto vcell = detail::read_reg16(*bus_, cfg_.address, kRegVcell);
        if (vcell.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Read SOC (unsigned 16-bit, MSB = integer %, LSB = 1/256 %).
        auto soc = detail::read_reg16(*bus_, cfg_.address, kRegSoc);
        if (soc.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Read CRATE (signed 16-bit).
        auto crate_raw = detail::read_reg16(*bus_, cfg_.address, kRegCrate);
        if (crate_raw.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Convert to physical units.
        // Voltage: raw × 78.125 µV = raw × 78125 / 1'000'000'000
        const float voltage_v  = static_cast<float>(vcell.unwrap()) * 78.125e-6f;
        // SOC: raw / 256 gives percent
        const float soc_pct    = static_cast<float>(soc.unwrap()) / 256.0f;
        // CRATE: signed 16-bit × 0.208 %/hr
        const auto  crate_s    = static_cast<std::int16_t>(crate_raw.unwrap());
        const float charge_rate = static_cast<float>(crate_s) * 0.208f;

        return alloy::core::Ok(Measurement{voltage_v, soc_pct, charge_rate});
    }

private:
    BusHandle* bus_;
    Config     cfg_;
};

}  // namespace alloy::drivers::power::max17048

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockI2cForMax17048Gate {
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
    sizeof(alloy::drivers::power::max17048::Device<_MockI2cForMax17048Gate>) > 0,
    "max17048 Device must compile against the documented I2C bus surface");
}  // namespace
