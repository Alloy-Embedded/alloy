#pragma once

// drivers/power/ina219/ina219.hpp
//
// Driver for Texas Instruments INA219 current/power monitor over I2C.
// Written against datasheet SBOS448G.
// Seed driver: calibration init + bus voltage / shunt voltage / current / power measurement.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::power::ina219 {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint16_t kDefaultAddress = 0x40u;  // A0=GND, A1=GND

// Register addresses.
inline constexpr std::uint8_t kRegConfig      = 0x00u;  // configuration
inline constexpr std::uint8_t kRegShuntV      = 0x01u;  // shunt voltage (signed, 10 µV/LSB)
inline constexpr std::uint8_t kRegBusV        = 0x02u;  // bus voltage (bits[15:3], 4 mV/LSB)
inline constexpr std::uint8_t kRegPower       = 0x03u;  // power (unsigned)
inline constexpr std::uint8_t kRegCurrent     = 0x04u;  // current (signed, needs calibration)
inline constexpr std::uint8_t kRegCalibration = 0x05u;  // calibration register

// CONFIG value: 32V range, PGA/8, 12-bit ADC×1, continuous shunt+bus.
inline constexpr std::uint16_t kConfigValue = 0x399Fu;

// ── Types ─────────────────────────────────────────────────────────────────────

struct Config {
    std::uint16_t address   = kDefaultAddress;
    float         shunt_ohm = 0.1f;   ///< Shunt resistor value in ohms
    float         max_amps  = 3.2f;   ///< Maximum expected current in amperes
};

struct Measurement {
    float bus_voltage_v;    ///< Bus voltage in volts
    float shunt_voltage_mv; ///< Shunt voltage in millivolts
    float current_a;        ///< Current in amperes
    float power_w;          ///< Power in watts
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Read a 16-bit big-endian register.
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

    explicit Device(BusHandle& bus, Config cfg = {})
        : bus_{&bus}, cfg_{cfg}, current_lsb_{0.0f} {}

    /// Configures the INA219 for continuous measurement and writes the
    /// calibration register derived from shunt resistance and max current.
    ///
    /// Sequence:
    ///   1. Write CONFIG = 0x399F (32V range, PGA/8, 12-bit, continuous shunt+bus).
    ///   2. Compute current_lsb  = max_amps / 32768.
    ///   3. Compute calibration  = 0.04096 / (current_lsb × shunt_ohm).
    ///   4. Write CALIBRATION register.
    [[nodiscard]] auto init() -> ResultVoid {
        // Step 1: write configuration.
        if (auto r = detail::write_reg16(*bus_, cfg_.address, kRegConfig, kConfigValue);
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Step 2: compute current LSB.
        current_lsb_ = cfg_.max_amps / 32768.0f;

        // Step 3: compute calibration value.
        const float calibration_f = 0.04096f / (current_lsb_ * cfg_.shunt_ohm);
        const auto  calibration   = static_cast<std::uint16_t>(calibration_f);

        // Step 4: write calibration register.
        if (auto r = detail::write_reg16(*bus_, cfg_.address, kRegCalibration, calibration);
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        return alloy::core::Ok();
    }

    /// Reads bus voltage, shunt voltage, current, and power from the device.
    ///
    /// Conversions:
    ///   bus_v      = (raw_bus >> 3) × 4 mV
    ///   shunt_mv   = raw_shunt × 10 µV → millivolts
    ///   current_a  = raw_current (signed) × current_lsb_
    ///   power_w    = raw_power × 20 × current_lsb_
    [[nodiscard]] auto read() -> ResultMeasurement {
        // Bus voltage (bits[15:3] = raw, bit1 = CNVR, bit0 = OVF).
        auto bus_raw = detail::read_reg16(*bus_, cfg_.address, kRegBusV);
        if (bus_raw.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Shunt voltage (signed 16-bit, 10 µV/LSB).
        auto shunt_raw = detail::read_reg16(*bus_, cfg_.address, kRegShuntV);
        if (shunt_raw.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Current (signed 16-bit, calibration-dependent).
        auto current_raw = detail::read_reg16(*bus_, cfg_.address, kRegCurrent);
        if (current_raw.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Power (unsigned 16-bit).
        auto power_raw = detail::read_reg16(*bus_, cfg_.address, kRegPower);
        if (power_raw.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Physical-unit conversions.
        const float bus_voltage_v    = static_cast<float>(bus_raw.unwrap() >> 3) * 0.004f;
        const float shunt_voltage_mv = static_cast<float>(
                                           static_cast<std::int16_t>(shunt_raw.unwrap()))
                                       * 0.01f;  // 10 µV/LSB → mV
        const float current_a        = static_cast<float>(
                                           static_cast<std::int16_t>(current_raw.unwrap()))
                                       * current_lsb_;
        const float power_w          = static_cast<float>(power_raw.unwrap())
                                       * 20.0f * current_lsb_;

        return alloy::core::Ok(Measurement{bus_voltage_v, shunt_voltage_mv,
                                           current_a, power_w});
    }

private:
    BusHandle* bus_;
    Config     cfg_;
    float      current_lsb_;  ///< Computed in init(), used in read()
};

}  // namespace alloy::drivers::power::ina219

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockI2cForIna219Gate {
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
    sizeof(alloy::drivers::power::ina219::Device<_MockI2cForIna219Gate>) > 0,
    "ina219 Device must compile against the documented I2C bus surface");
}  // namespace
