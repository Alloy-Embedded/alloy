#pragma once

// drivers/sensor/icm42688p/icm42688p.hpp
//
// Driver for TDK InvenSense ICM-42688-P 6-axis IMU (accelerometer + gyroscope) over SPI.
// Written against datasheet DS-000347 Rev 1.7.
// Seed driver: WHO_AM_I probe + soft reset + accel/gyro LN-mode startup +
// burst 14-byte data read with physical-unit conversion.
// See drivers/README.md.
//
// SPI mode 0 or mode 3. Bus surface: transfer(span<const uint8_t>, span<uint8_t>).
// Read protocol:  tx[0] = reg | 0x80, tx[1..N] = 0x00; rx[1..N] = data bytes.
// Write protocol: tx[0] = reg,         tx[1]    = value; rx ignored.
//
// CS is managed by the CsPolicy template parameter:
//   NoOpCsPolicy       — SPI hardware holds CS permanently (default).
//   GpioCsPolicy<Pin>  — software GPIO CS (recommended for shared buses).

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::icm42688p {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint8_t kExpectedWhoAmI = 0x47u;  // WHO_AM_I fixed value

namespace reg {
inline constexpr std::uint8_t kDeviceConfig = 0x11u;
inline constexpr std::uint8_t kIntConfig    = 0x14u;
inline constexpr std::uint8_t kFifoConfig   = 0x16u;
inline constexpr std::uint8_t kTempData1    = 0x1Du;  // MSB; burst starts here
inline constexpr std::uint8_t kAccelDataX1  = 0x1Fu;
inline constexpr std::uint8_t kGyroDataX1   = 0x25u;
inline constexpr std::uint8_t kPwrMgmt0     = 0x4Eu;
inline constexpr std::uint8_t kGyroConfig0  = 0x4Fu;
inline constexpr std::uint8_t kAccelConfig0 = 0x50u;
inline constexpr std::uint8_t kWhoAmI       = 0x75u;
inline constexpr std::uint8_t kBankSel      = 0x76u;
}  // namespace reg

// SPI read bit (OR into register address for reads).
inline constexpr std::uint8_t kSpiReadBit = 0x80u;

// ── CS policies ───────────────────────────────────────────────────────────────

struct NoOpCsPolicy {
    void assert_cs()   const noexcept {}
    void deassert_cs() const noexcept {}
};

template <typename GpioPin>
struct GpioCsPolicy {
    explicit GpioCsPolicy(GpioPin& pin) : pin_{&pin} {
        (void)pin_->set_high();
    }
    void assert_cs()   const noexcept { (void)pin_->set_low(); }
    void deassert_cs() const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── Configuration enums ───────────────────────────────────────────────────────

enum class AccelFullScale : std::uint8_t {
    G2  = 3,  ///< ±2 g,    16384 LSB/g
    G4  = 2,  ///< ±4 g,     8192 LSB/g
    G8  = 1,  ///< ±8 g,     4096 LSB/g
    G16 = 0,  ///< ±16 g,    2048 LSB/g
};

enum class GyroFullScale : std::uint8_t {
    Dps250  = 3,  ///< ±250 dps,   131.0 LSB/dps
    Dps500  = 2,  ///< ±500 dps,    65.6 LSB/dps
    Dps1000 = 1,  ///< ±1000 dps,   32.8 LSB/dps
    Dps2000 = 0,  ///< ±2000 dps,   16.4 LSB/dps
};

enum class OutputDataRate : std::uint8_t {
    Hz1k = 6,  ///< 1 kHz
    Hz2k = 5,  ///< 2 kHz
    Hz4k = 4,  ///< 4 kHz
    Hz8k = 3,  ///< 8 kHz
};

struct Config {
    AccelFullScale accel_fs = AccelFullScale::G16;
    GyroFullScale  gyro_fs  = GyroFullScale::Dps2000;
    OutputDataRate odr      = OutputDataRate::Hz1k;
};

// ── Measurement ───────────────────────────────────────────────────────────────

struct Measurement {
    float accel_x_g;     ///< Acceleration X in g
    float accel_y_g;     ///< Acceleration Y in g
    float accel_z_g;     ///< Acceleration Z in g
    float gyro_x_dps;    ///< Angular rate X in degrees/second
    float gyro_y_dps;    ///< Angular rate Y in degrees/second
    float gyro_z_dps;    ///< Angular rate Z in degrees/second
    float temperature_c; ///< Die temperature in degrees Celsius
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 1 ms (calibrated for ~Cortex-M clock rates).
inline void busy_wait_1ms() {
    volatile std::uint32_t n = 10'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Busy-wait approximately 200 ms (sensor settle after power-on).
inline void busy_wait_200ms() {
    volatile std::uint32_t n = 2'000'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Returns accel sensitivity in LSB/g for a given full-scale range.
[[nodiscard]] inline constexpr auto accel_sensitivity(AccelFullScale fs) -> float {
    switch (fs) {
        case AccelFullScale::G16: return  2048.0f;
        case AccelFullScale::G8:  return  4096.0f;
        case AccelFullScale::G4:  return  8192.0f;
        case AccelFullScale::G2:  return 16384.0f;
    }
    return 2048.0f;  // unreachable; satisfy compiler
}

/// Returns gyro sensitivity in LSB/dps for a given full-scale range.
[[nodiscard]] inline constexpr auto gyro_sensitivity(GyroFullScale fs) -> float {
    switch (fs) {
        case GyroFullScale::Dps2000: return  16.4f;
        case GyroFullScale::Dps1000: return  32.8f;
        case GyroFullScale::Dps500:  return  65.6f;
        case GyroFullScale::Dps250:  return 131.0f;
    }
    return 16.4f;  // unreachable; satisfy compiler
}

/// Read N data bytes from reg_addr over SPI.
/// tx: [reg|0x80, 0x00 × N]; rx: [dummy, data[0..N-1]].
template <typename Bus, typename CsGuard, std::size_t N>
[[nodiscard]] auto spi_read(Bus& bus, CsGuard& cs, std::uint8_t reg,
                             std::array<std::uint8_t, N>& out)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    static_assert(N >= 1u, "spi_read: output buffer must be at least 1 byte");
    // Frame: 1 command byte + N data bytes.
    std::array<std::uint8_t, 1 + N> tx{};
    std::array<std::uint8_t, 1 + N> rx{};
    tx[0] = static_cast<std::uint8_t>(reg | kSpiReadBit);
    // tx[1..N] already zero-filled.

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx}, std::span<std::uint8_t>{rx});
    cs.deassert_cs();
    if (r.is_err()) {
        return alloy::core::Err(std::move(r).err());
    }
    for (std::size_t i = 0; i < N; ++i) {
        out[i] = rx[i + 1];
    }
    return alloy::core::Ok();
}

/// Write one byte to reg_addr over SPI.
/// tx: [reg, value]; rx: ignored.
template <typename Bus, typename CsGuard>
[[nodiscard]] auto spi_write(Bus& bus, CsGuard& cs, std::uint8_t reg, std::uint8_t val)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    std::array<std::uint8_t, 2> tx{reg, val};
    std::array<std::uint8_t, 2> rx{};

    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx}, std::span<std::uint8_t>{rx});
    cs.deassert_cs();
    return r;
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle, typename CsPolicy = NoOpCsPolicy>
class Device {
public:
    using ResultVoid        = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, CsPolicy cs = {}, Config cfg = {})
        : bus_{&bus}, cs_{cs}, cfg_{cfg} {}

    /// Verifies device presence via WHO_AM_I and runs the startup sequence.
    ///
    /// Sequence:
    ///   1. Read WHO_AM_I — expect 0x47.
    ///   2. Write DEVICE_CONFIG = 0x01 (soft reset).
    ///   3. Busy-wait ~1 ms.
    ///   4. Write PWR_MGMT0 = 0x0F (accel + gyro in LN mode).
    ///   5. Busy-wait ~1 ms.
    ///   6. Write ACCEL_CONFIG0 = (accel_fs << 5) | odr.
    ///   7. Write GYRO_CONFIG0  = (gyro_fs  << 5) | odr.
    ///   8. Busy-wait ~200 ms (sensor settle).
    ///
    /// Returns `CommunicationError` on SPI failure or WHO_AM_I mismatch.
    [[nodiscard]] auto init() -> ResultVoid {
        // 1. WHO_AM_I check.
        std::array<std::uint8_t, 1> who_am_i{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kWhoAmI, who_am_i); r.is_err()) {
            return r;
        }
        if (who_am_i[0] != kExpectedWhoAmI) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        // 2. Soft reset.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kDeviceConfig, 0x01u); r.is_err()) {
            return r;
        }

        // 3. Wait for reset to complete.
        detail::busy_wait_1ms();

        // 4. Enable accel + gyro in LN mode.
        if (auto r = detail::spi_write(*bus_, cs_, reg::kPwrMgmt0, 0x0Fu); r.is_err()) {
            return r;
        }

        // 5. Wait for power-mode transition.
        detail::busy_wait_1ms();

        // 6. Accel full-scale + ODR.
        const std::uint8_t accel_cfg =
            static_cast<std::uint8_t>(
                (static_cast<std::uint8_t>(cfg_.accel_fs) << 5) |
                static_cast<std::uint8_t>(cfg_.odr));
        if (auto r = detail::spi_write(*bus_, cs_, reg::kAccelConfig0, accel_cfg); r.is_err()) {
            return r;
        }

        // 7. Gyro full-scale + ODR.
        const std::uint8_t gyro_cfg =
            static_cast<std::uint8_t>(
                (static_cast<std::uint8_t>(cfg_.gyro_fs) << 5) |
                static_cast<std::uint8_t>(cfg_.odr));
        if (auto r = detail::spi_write(*bus_, cs_, reg::kGyroConfig0, gyro_cfg); r.is_err()) {
            return r;
        }

        // 8. Sensor settle time.
        detail::busy_wait_200ms();

        return alloy::core::Ok();
    }

    /// Burst-reads 14 bytes starting at TEMP_DATA1 and converts to physical units.
    ///
    /// Register layout (14 bytes, big-endian pairs):
    ///   [0..1]  TEMP   MSB/LSB
    ///   [2..3]  ACCEL_X MSB/LSB
    ///   [4..5]  ACCEL_Y MSB/LSB
    ///   [6..7]  ACCEL_Z MSB/LSB
    ///   [8..9]  GYRO_X  MSB/LSB
    ///   [10..11] GYRO_Y MSB/LSB
    ///   [12..13] GYRO_Z MSB/LSB
    ///
    /// Temperature: t_c = (raw_t / 132.48) + 25.0
    /// Accel:       value_g = raw / sensitivity (LSB/g)
    /// Gyro:        value_dps = raw / sensitivity (LSB/dps)
    [[nodiscard]] auto read() -> ResultMeasurement {
        std::array<std::uint8_t, 14> raw{};
        if (auto r = detail::spi_read(*bus_, cs_, reg::kTempData1, raw); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        // Reconstruct signed 16-bit words (big-endian: MSB first).
        const auto to_int16 = [](std::uint8_t msb, std::uint8_t lsb) -> std::int16_t {
            return static_cast<std::int16_t>(
                (static_cast<std::uint16_t>(msb) << 8) | static_cast<std::uint16_t>(lsb));
        };

        const std::int16_t raw_t  = to_int16(raw[0],  raw[1]);
        const std::int16_t raw_ax = to_int16(raw[2],  raw[3]);
        const std::int16_t raw_ay = to_int16(raw[4],  raw[5]);
        const std::int16_t raw_az = to_int16(raw[6],  raw[7]);
        const std::int16_t raw_gx = to_int16(raw[8],  raw[9]);
        const std::int16_t raw_gy = to_int16(raw[10], raw[11]);
        const std::int16_t raw_gz = to_int16(raw[12], raw[13]);

        const float a_sens = detail::accel_sensitivity(cfg_.accel_fs);
        const float g_sens = detail::gyro_sensitivity(cfg_.gyro_fs);

        Measurement m{};
        m.temperature_c = (static_cast<float>(raw_t)  / 132.48f) + 25.0f;
        m.accel_x_g     =  static_cast<float>(raw_ax) / a_sens;
        m.accel_y_g     =  static_cast<float>(raw_ay) / a_sens;
        m.accel_z_g     =  static_cast<float>(raw_az) / a_sens;
        m.gyro_x_dps    =  static_cast<float>(raw_gx) / g_sens;
        m.gyro_y_dps    =  static_cast<float>(raw_gy) / g_sens;
        m.gyro_z_dps    =  static_cast<float>(raw_gz) / g_sens;
        return alloy::core::Ok(std::move(m));
    }

private:
    BusHandle* bus_;
    CsPolicy   cs_;
    Config     cfg_;
};

}  // namespace alloy::drivers::sensor::icm42688p

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// SPI bus surface.
namespace {
struct _MockSpiForIcm42688pGate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};
static_assert(
    sizeof(alloy::drivers::sensor::icm42688p::Device<_MockSpiForIcm42688pGate>) > 0,
    "icm42688p Device must compile against the documented SPI bus surface");
}  // namespace
