/// Core units and type-safe wrappers
///
/// Provides compile-time type safety for physical units.

#ifndef ALLOY_CORE_UNITS_HPP
#define ALLOY_CORE_UNITS_HPP

#include "types.hpp"

namespace alloy::core {

/// Type-safe baud rate wrapper
///
/// Prevents mixing baud rates with raw integers and enables compile-time
/// validation of common baud rates.
class BaudRate {
public:
    /// Construct from raw baud rate value
    constexpr explicit BaudRate(u32 rate) : rate_(rate) {}

    /// Get raw baud rate value
    [[nodiscard]] constexpr u32 value() const noexcept { return rate_; }

    /// Comparison operators
    [[nodiscard]] constexpr bool operator==(const BaudRate& other) const noexcept {
        return rate_ == other.rate_;
    }
    [[nodiscard]] constexpr bool operator!=(const BaudRate& other) const noexcept {
        return rate_ != other.rate_;
    }

private:
    u32 rate_;
};

/// User-defined literals for common baud rates
namespace literals {

/// 9600 baud
constexpr BaudRate operator""_baud(unsigned long long rate) {
    return BaudRate(static_cast<u32>(rate));
}

} // namespace literals

/// Common baud rate constants
namespace baud_rates {
    constexpr BaudRate Baud9600   = BaudRate(9600);
    constexpr BaudRate Baud19200  = BaudRate(19200);
    constexpr BaudRate Baud38400  = BaudRate(38400);
    constexpr BaudRate Baud57600  = BaudRate(57600);
    constexpr BaudRate Baud115200 = BaudRate(115200);
    constexpr BaudRate Baud230400 = BaudRate(230400);
    constexpr BaudRate Baud460800 = BaudRate(460800);
    constexpr BaudRate Baud921600 = BaudRate(921600);
} // namespace baud_rates

} // namespace alloy::core

#endif // ALLOY_CORE_UNITS_HPP
