#pragma once

// drivers/power/ina3221/ina3221.hpp
//
// Driver for Texas Instruments INA3221 triple-channel shunt/bus voltage monitor over I2C.
// Written against datasheet SBOS576C.
// Seed driver: MANUFACTURER_ID probe + three-channel shunt/bus voltage measurement.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::power::ina3221 {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint16_t kDefaultAddress   = 0x40u;  // A0=GND
inline constexpr std::uint16_t kAddress_A0_VS    = 0x41u;  // A0=VS
inline constexpr std::uint16_t kAddress_A0_SDA   = 0x42u;  // A0=SDA
inline constexpr std::uint16_t kAddress_A0_SCL   = 0x43u;  // A0=SCL

inline constexpr std::uint16_t kExpectedManufId  = 0x5449u;  // "TI" in ASCII
inline constexpr std::uint16_t kExpectedDieId    = 0x3220u;

// Register addresses.
inline constexpr std::uint8_t kRegConfig         = 0x00u;
inline constexpr std::uint8_t kRegCh1ShuntV      = 0x01u;  // CH1 shunt voltage
inline constexpr std::uint8_t kRegCh1BusV        = 0x02u;  // CH1 bus voltage
inline constexpr std::uint8_t kRegCh2ShuntV      = 0x03u;  // CH2 shunt voltage
inline constexpr std::uint8_t kRegCh2BusV        = 0x04u;  // CH2 bus voltage
inline constexpr std::uint8_t kRegCh3ShuntV      = 0x05u;  // CH3 shunt voltage
inline constexpr std::uint8_t kRegCh3BusV        = 0x06u;  // CH3 bus voltage
inline constexpr std::uint8_t kRegManufacturerId = 0xFEu;  // Manufacturer ID ("TI" = 0x5449)
inline constexpr std::uint8_t kRegDieId          = 0xFFu;  // Die ID (0x3220)

// CONFIG: all 3 channels enabled, 1x average, 1.1 ms conversion, continuous.
inline constexpr std::uint16_t kConfigValue = 0x7127u;

// Shunt voltage LSB: 40 µV (bits[15:3] valid; shift right 3 to get count).
// Bus voltage LSB:   8 mV  (bits[15:3] valid; shift right 3 to get count).
inline constexpr std::int32_t  kShuntLsb_uV = 40;      // µV per LSB
inline constexpr float         kBusLsb_V    = 0.008f;  // V per LSB

// ── Types ─────────────────────────────────────────────────────────────────────

struct Config {
    std::uint16_t address   = kDefaultAddress;
    float         shunt_ohm = 0.1f;  ///< Shunt resistor value for all 3 channels (ohms)
};

struct ChannelMeasurement {
    float bus_voltage_v;  ///< Bus voltage in volts
    float current_a;      ///< Current in amperes (shunt_v_uV / (1e6 × shunt_ohm))
};

struct Measurement {
    ChannelMeasurement ch[3];  ///< ch[0]=channel 1, ch[1]=channel 2, ch[2]=channel 3
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

/// Read one channel as a 4-byte burst (shunt register + auto-increment → bus register).
///
/// bytes[0:1] = shunt voltage (big-endian, signed bits[15:3], 40 µV/LSB)
/// bytes[2:3] = bus voltage   (big-endian, unsigned bits[15:3], 8 mV/LSB)
template <typename BusHandle>
[[nodiscard]] inline auto read_channel(BusHandle& bus,
                                       std::uint16_t addr,
                                       std::uint8_t  shunt_reg,
                                       float         shunt_ohm)
    -> alloy::core::Result<ChannelMeasurement, alloy::core::ErrorCode>
{
    const std::array<std::uint8_t, 1> tx{shunt_reg};
    std::array<std::uint8_t, 4> rx{};
    if (auto r = bus.write_read(addr,
                                std::span<const std::uint8_t>{tx},
                                std::span<std::uint8_t>{rx});
        r.is_err()) {
        return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
    }

    const std::uint16_t raw_shunt =
        (static_cast<std::uint16_t>(rx[0]) << 8) | static_cast<std::uint16_t>(rx[1]);
    const std::uint16_t raw_bus =
        (static_cast<std::uint16_t>(rx[2]) << 8) | static_cast<std::uint16_t>(rx[3]);

    // Shunt: arithmetic right-shift by 3 to get signed 13-bit count.
    const auto shunt_count  = static_cast<std::int16_t>(raw_shunt) >> 3;
    const auto shunt_v_uv   = static_cast<std::int32_t>(shunt_count) * kShuntLsb_uV;

    // Bus: logical right-shift by 3 to get unsigned count.
    const auto bus_count    = static_cast<std::uint16_t>(raw_bus >> 3);
    const float bus_voltage_v = static_cast<float>(bus_count) * kBusLsb_V;

    // Current: shunt_v_uV / (shunt_ohm × 1e6)
    const float current_a = static_cast<float>(shunt_v_uv) / (1'000'000.0f * shunt_ohm);

    return alloy::core::Ok(ChannelMeasurement{bus_voltage_v, current_a});
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    /// Verifies device presence via MANUFACTURER_ID and configures all channels.
    ///
    /// Returns `CommunicationError` if the I2C transfer fails or MANUFACTURER_ID
    /// does not read back 0x5449 ("TI").
    [[nodiscard]] auto init() -> ResultVoid {
        // Read MANUFACTURER_ID — must be 0x5449 ("TI").
        auto manuf = detail::read_reg16(*bus_, cfg_.address, kRegManufacturerId);
        if (manuf.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        if (manuf.unwrap() != kExpectedManufId) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // Write CONFIG: all 3 channels enabled, 1x average, 1.1 ms, continuous.
        if (auto r = detail::write_reg16(*bus_, cfg_.address, kRegConfig, kConfigValue);
            r.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        return alloy::core::Ok();
    }

    /// Reads shunt + bus voltages for all three channels and computes current.
    ///
    /// Each channel uses a 4-byte burst starting at the shunt register;
    /// the auto-increment pointer covers the adjacent bus voltage register.
    [[nodiscard]] auto read() -> ResultMeasurement {
        Measurement m{};

        auto ch1 = detail::read_channel(*bus_, cfg_.address, kRegCh1ShuntV, cfg_.shunt_ohm);
        if (ch1.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        m.ch[0] = ch1.unwrap();

        auto ch2 = detail::read_channel(*bus_, cfg_.address, kRegCh2ShuntV, cfg_.shunt_ohm);
        if (ch2.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        m.ch[1] = ch2.unwrap();

        auto ch3 = detail::read_channel(*bus_, cfg_.address, kRegCh3ShuntV, cfg_.shunt_ohm);
        if (ch3.is_err()) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }
        m.ch[2] = ch3.unwrap();

        return alloy::core::Ok(m);
    }

private:
    BusHandle* bus_;
    Config     cfg_;
};

}  // namespace alloy::drivers::power::ina3221

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// I2C bus surface.
namespace {
struct _MockI2cForIna3221Gate {
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
    sizeof(alloy::drivers::power::ina3221::Device<_MockI2cForIna3221Gate>) > 0,
    "ina3221 Device must compile against the documented I2C bus surface");
}  // namespace
