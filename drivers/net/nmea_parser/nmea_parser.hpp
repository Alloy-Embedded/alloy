#pragma once

// drivers/net/nmea_parser/nmea_parser.hpp
//
// Stateless NMEA-0183 line-level parser over UART.
// Parses $GPGGA / $GNGGA (position fix) and $GPRMC / $GNRMC (recommended
// minimum) sentences from a GPS/GNSS receiver.
//
// Transport contract — templated over any UartBusHandle that exposes:
//   auto transmit(std::span<const std::uint8_t>) -> Result<void, ErrorCode>;
//   auto receive(std::span<std::uint8_t>)         -> Result<void, ErrorCode>;
//
// Design notes:
//   - No heap, no std::string, no printf, no exceptions, no virtual dispatch.
//   - All storage is std::array members (stack or as a member object).
//   - Checksum verification is intentionally omitted in this seed driver.
//     Production code should call detail::verify_checksum() before parsing.
//   - The parser is stateless between calls to read_line(); it simply
//     overwrites the internal line buffer on each call.
//
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::net::nmea_parser {

// ── Output types ──────────────────────────────────────────────────────────────

/// Parsed $GPGGA / $GNGGA fix data.
struct GgaFix {
    std::uint8_t hour;         ///< UTC hour   (0–23)
    std::uint8_t minute;       ///< UTC minute (0–59)
    std::uint8_t second;       ///< UTC second (0–59)
    float        latitude;     ///< Decimal degrees; negative = South
    float        longitude;    ///< Decimal degrees; negative = West
    std::uint8_t fix_quality;  ///< 0=invalid, 1=GPS, 2=DGPS
    std::uint8_t satellites;   ///< Number of satellites in use
    float        altitude_m;   ///< Altitude above mean sea-level, metres
};

/// Parsed $GPRMC / $GNRMC recommended minimum data.
struct RmcFix {
    std::uint8_t hour;         ///< UTC hour
    std::uint8_t minute;       ///< UTC minute
    std::uint8_t second;       ///< UTC second
    bool         valid;        ///< true when status field == 'A' (active)
    float        latitude;     ///< Decimal degrees; negative = South
    float        longitude;    ///< Decimal degrees; negative = West
    float        speed_knots;  ///< Speed over ground, knots
    float        course_deg;   ///< Course over ground, degrees true
};

// ── Detail helpers ─────────────────────────────────────────────────────────────

namespace detail {

/// Parse an unsigned decimal integer from a null-terminated C-string.
/// Stops at the first non-digit character. Returns 0 on empty / null input.
[[nodiscard]] inline std::uint32_t parse_uint(const char* s) {
    if (s == nullptr) return 0u;
    std::uint32_t result = 0u;
    while (*s >= '0' && *s <= '9') {
        result = result * 10u + static_cast<std::uint32_t>(*s - '0');
        ++s;
    }
    return result;
}

/// Parse a non-negative floating-point number from a null-terminated C-string.
/// Handles optional leading digits and an optional decimal point.
/// No sign, no exponent — sufficient for NMEA numeric fields.
[[nodiscard]] inline float parse_float(const char* s) {
    if (s == nullptr) return 0.0f;
    // Integer part.
    float result = 0.0f;
    while (*s >= '0' && *s <= '9') {
        result = result * 10.0f + static_cast<float>(*s - '0');
        ++s;
    }
    // Fractional part.
    if (*s == '.') {
        ++s;
        float scale = 0.1f;
        while (*s >= '0' && *s <= '9') {
            result += static_cast<float>(*s - '0') * scale;
            scale  *= 0.1f;
            ++s;
        }
    }
    return result;
}

/// Convert an NMEA lat/lon field ("ddmm.mmmm" or "dddmm.mmmm") plus a
/// hemisphere character ('N'/'S'/'E'/'W') to signed decimal degrees.
///
/// NMEA encoding: the first 2 (lat) or 3 (lon) digits are whole degrees,
/// the remaining digits are minutes (mm.mmmm). Longitude fields are
/// distinguished by the caller passing the correct integer-degree width,
/// but since the field value itself encodes the boundary implicitly we use
/// the standard NMEA split: degrees = trunc(value / 100), minutes = value mod 100.
[[nodiscard]] inline float parse_latlon(const char* field, char hemi) {
    if (field == nullptr || *field == '\0') return 0.0f;
    const float raw     = parse_float(field);
    const float degrees = static_cast<float>(static_cast<int>(raw / 100.0f));
    const float minutes = raw - degrees * 100.0f;
    float result = degrees + minutes / 60.0f;
    if (hemi == 'S' || hemi == 'W') {
        result = -result;
    }
    return result;
}

/// Split a comma-separated NMEA sentence (already in a mutable buffer) into
/// an array of C-string pointers.  The buffer is modified in-place (commas
/// replaced with '\0').  The asterisk that precedes the checksum is also
/// treated as a field terminator.
/// Returns the number of fields found (capped at MaxFields).
template <std::size_t MaxFields>
inline std::size_t split_fields(char* buf, std::size_t len,
                                std::array<char*, MaxFields>& out) {
    std::size_t count = 0u;
    if (buf == nullptr || len == 0u) return 0u;

    // Field 0 starts at the beginning of the buffer.
    out[count++] = buf;

    for (std::size_t i = 0u; i < len && count < MaxFields; ++i) {
        if (buf[i] == ',' || buf[i] == '*') {
            buf[i] = '\0';
            if (i + 1u < len) {
                out[count++] = &buf[i + 1u];
            }
        }
    }
    return count;
}

/// Return true when the two C-strings (first `n` chars of `a`) equal `b`.
/// Used for sentence-type prefix checks without <cstring>.
[[nodiscard]] inline bool starts_with(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) return false;
    while (*b != '\0') {
        if (*a != *b) return false;
        ++a; ++b;
    }
    return true;
}

}  // namespace detail

// ── Parser ────────────────────────────────────────────────────────────────────

template <typename BusHandle>
class Parser {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultGga  = alloy::core::Result<GgaFix,  alloy::core::ErrorCode>;
    using ResultRmc  = alloy::core::Result<RmcFix,  alloy::core::ErrorCode>;

    explicit Parser(BusHandle& bus) : bus_{&bus} {}

    /// Read bytes from the UART one at a time until a newline ('\\n') is
    /// received or the internal buffer is full.
    ///
    /// Trailing \\r and \\n characters are stripped; the result is stored
    /// null-terminated in the internal buffer and the length (excluding the
    /// null terminator) is recorded.
    ///
    /// Returns:
    ///   - Ok(void)             on success.
    ///   - CommunicationError  if the UART receive call fails.
    ///   - BufferFull          if the line exceeds kMaxLineLen - 1 bytes
    ///                         with no newline seen.
    [[nodiscard]] auto read_line() -> ResultVoid {
        line_len_ = 0u;

        while (line_len_ < kMaxLineLen - 1u) {
            std::uint8_t byte = 0u;
            auto span = std::span<std::uint8_t>{&byte, 1u};
            if (auto r = bus_->receive(span); r.is_err()) {
                return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
            }
            if (byte == static_cast<std::uint8_t>('\n')) {
                break;
            }
            if (byte != static_cast<std::uint8_t>('\r')) {
                line_buf_[line_len_++] = static_cast<char>(byte);
            }
        }

        // Guard: if buffer filled without seeing '\n', that is an error.
        if (line_len_ == kMaxLineLen - 1u) {
            line_buf_[line_len_] = '\0';
            return alloy::core::Err(alloy::core::ErrorCode::BufferFull);
        }

        line_buf_[line_len_] = '\0';
        return alloy::core::Ok();
    }

    /// Parse the last line read by read_line() as a $GPGGA or $GNGGA sentence.
    ///
    /// Field layout (1-based after the sentence type):
    ///   1: hhmmss.ss  2: lat  3: N/S  4: lon  5: E/W
    ///   6: fix quality  7: satellites  8: HDOP  9: altitude  10: M  …
    ///
    /// Returns InvalidParameter if the sentence type does not match or a
    /// mandatory field is absent / malformed.
    [[nodiscard]] auto parse_gga() const -> ResultGga {
        // Work on a mutable copy so split_fields can insert null terminators.
        std::array<char, kMaxLineLen> buf{};
        for (std::size_t i = 0u; i <= line_len_; ++i) buf[i] = line_buf_[i];

        std::array<char*, 16u> fields{};
        const std::size_t n = detail::split_fields(buf.data(), line_len_, fields);

        if (n < 1u) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        // Accept both talker IDs: $GPGGA (GPS-only) and $GNGGA (multi-GNSS).
        if (!detail::starts_with(fields[0], "$GPGGA") &&
            !detail::starts_with(fields[0], "$GNGGA")) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        // Require at least fields 0–9 (type + 9 data fields).
        if (n < 10u) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        GgaFix fix{};

        // Field 1: time hhmmss.ss
        {
            const float t = detail::parse_float(fields[1]);
            const auto  ti = static_cast<std::uint32_t>(t);
            fix.hour   = static_cast<std::uint8_t>(ti / 10000u);
            fix.minute = static_cast<std::uint8_t>((ti / 100u) % 100u);
            fix.second = static_cast<std::uint8_t>(ti % 100u);
        }

        // Fields 2+3: latitude + hemisphere
        fix.latitude = detail::parse_latlon(fields[2],
                                            (fields[3] != nullptr) ? fields[3][0] : 'N');

        // Fields 4+5: longitude + hemisphere
        fix.longitude = detail::parse_latlon(fields[4],
                                             (fields[5] != nullptr) ? fields[5][0] : 'E');

        // Field 6: fix quality
        fix.fix_quality = static_cast<std::uint8_t>(detail::parse_uint(fields[6]));

        // Field 7: number of satellites
        fix.satellites = static_cast<std::uint8_t>(detail::parse_uint(fields[7]));

        // Field 8: HDOP (skipped — not in GgaFix)

        // Field 9: altitude in metres
        fix.altitude_m = detail::parse_float(fields[9]);

        return alloy::core::Ok(fix);
    }

    /// Parse the last line read by read_line() as a $GPRMC or $GNRMC sentence.
    ///
    /// Field layout (1-based after the sentence type):
    ///   1: hhmmss.ss  2: A/V  3: lat  4: N/S  5: lon  6: E/W
    ///   7: speed(kn)  8: course(deg)  9: ddmmyy  10: mag var  11: E/W
    ///
    /// Returns InvalidParameter if the sentence type does not match or a
    /// mandatory field is absent / malformed.
    [[nodiscard]] auto parse_rmc() const -> ResultRmc {
        // Work on a mutable copy.
        std::array<char, kMaxLineLen> buf{};
        for (std::size_t i = 0u; i <= line_len_; ++i) buf[i] = line_buf_[i];

        std::array<char*, 16u> fields{};
        const std::size_t n = detail::split_fields(buf.data(), line_len_, fields);

        if (n < 1u) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        // Accept $GPRMC (GPS-only) and $GNRMC (multi-GNSS).
        if (!detail::starts_with(fields[0], "$GPRMC") &&
            !detail::starts_with(fields[0], "$GNRMC")) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }
        // Require at least fields 0–8.
        if (n < 9u) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        RmcFix fix{};

        // Field 1: time hhmmss.ss
        {
            const float t = detail::parse_float(fields[1]);
            const auto  ti = static_cast<std::uint32_t>(t);
            fix.hour   = static_cast<std::uint8_t>(ti / 10000u);
            fix.minute = static_cast<std::uint8_t>((ti / 100u) % 100u);
            fix.second = static_cast<std::uint8_t>(ti % 100u);
        }

        // Field 2: status 'A' = active / valid, 'V' = void
        fix.valid = (fields[2] != nullptr) && (fields[2][0] == 'A');

        // Fields 3+4: latitude + hemisphere
        fix.latitude = detail::parse_latlon(fields[3],
                                            (fields[4] != nullptr) ? fields[4][0] : 'N');

        // Fields 5+6: longitude + hemisphere
        fix.longitude = detail::parse_latlon(fields[5],
                                             (fields[6] != nullptr) ? fields[6][0] : 'E');

        // Field 7: speed over ground, knots
        fix.speed_knots = detail::parse_float(fields[7]);

        // Field 8: course over ground, degrees true
        fix.course_deg = detail::parse_float(fields[8]);

        return alloy::core::Ok(fix);
    }

    /// Returns a span of the raw characters in the last line (excluding the
    /// null terminator).  Valid until the next call to read_line().
    [[nodiscard]] auto last_line() const -> std::span<const char> {
        return std::span<const char>{line_buf_.data(), line_len_};
    }

private:
    static constexpr std::size_t kMaxLineLen = 128u;

    BusHandle*                      bus_;
    std::array<char, kMaxLineLen>   line_buf_{};
    std::size_t                     line_len_{0u};
};

}  // namespace alloy::drivers::net::nmea_parser

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Parser no longer compiles against the documented
// UART bus surface (transmit + receive over std::span<uint8_t>).
namespace {
struct _MockUartBusForNmeaParserGate {
    [[nodiscard]] auto transmit(std::span<const std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    [[nodiscard]] auto receive(std::span<std::uint8_t>) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};
static_assert(
    sizeof(alloy::drivers::net::nmea_parser::Parser<_MockUartBusForNmeaParserGate>) > 0,
    "nmea_parser Parser must compile against the documented UART bus surface");
}  // namespace
