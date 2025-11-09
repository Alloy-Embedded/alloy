/// Platform-agnostic UART interface
///
/// Defines UART concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_UART_HPP
#define ALLOY_HAL_INTERFACE_UART_HPP

#include <concepts>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "core/units.hpp"

namespace alloy::hal {

/// UART data bits configuration
enum class DataBits : core::u8 {
    Five = 5,
    Six = 6,
    Seven = 7,
    Eight = 8,
    Nine = 9  // Some MCUs support 9-bit mode
};

/// UART parity configuration
enum class Parity : core::u8 { None = 0, Even = 1, Odd = 2 };

/// UART stop bits configuration
enum class StopBits : core::u8 { One = 1, Two = 2 };

/// UART configuration parameters
///
/// Contains all parameters needed to configure a UART peripheral.
struct UartConfig {
    core::BaudRate baud_rate;
    DataBits data_bits = DataBits::Eight;
    Parity parity = Parity::None;
    StopBits stop_bits = StopBits::One;

    /// Constructor with default 8N1 configuration
    constexpr UartConfig(core::BaudRate rate, DataBits bits = DataBits::Eight,
                         Parity par = Parity::None, StopBits stop = StopBits::One)
        : baud_rate(rate),
          data_bits(bits),
          parity(par),
          stop_bits(stop) {}
};

/// UART device concept
///
/// Defines the interface that all UART implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
template <typename T>
concept UartDevice = requires(T device, const T const_device, core::u8 byte, UartConfig config) {
    /// Read a single byte from UART
    /// Returns ErrorCode::Timeout if no data available
    { device.read_byte() } -> std::same_as<core::Result<core::u8>>;

    /// Write a single byte to UART
    /// Returns ErrorCode::Busy if transmit buffer is full
    { device.write_byte(byte) } -> std::same_as<core::Result<void>>;

    /// Check how many bytes are available to read
    { const_device.available() } -> std::same_as<core::usize>;

    /// Configure the UART with new parameters
    /// Returns ErrorCode::InvalidParameter if configuration is invalid
    { device.configure(config) } -> std::same_as<core::Result<void>>;
};

/// Type alias for configured UART device
///
/// Wraps a UART device type with compile-time configuration.
template <typename UartImpl, core::u32 BaudRateValue>
    requires UartDevice<UartImpl>
class ConfiguredUart {
   public:
    ConfiguredUart() {
        // Configure on construction with compile-time rate
        UartConfig config{core::BaudRate{BaudRateValue}};
        device_.configure(config);
    }

    /// Read a byte from UART
    [[nodiscard]] core::Result<core::u8> read_byte() { return device_.read_byte(); }

    /// Write a byte to UART
    [[nodiscard]] core::Result<void> write_byte(core::u8 byte) { return device_.write_byte(byte); }

    /// Check available bytes
    [[nodiscard]] core::usize available() const { return device_.available(); }

    /// Write multiple bytes
    [[nodiscard]] core::Result<void> write(const core::u8* data, core::usize length) {
        for (core::usize i = 0; i < length; ++i) {
            auto result = device_.write_byte(data[i]);
            if (result.is_error()) {
                return result;
            }
        }
        return core::Ok();
    }

    /// Write null-terminated string
    [[nodiscard]] core::Result<void> write_str(const char* str) {
        while (*str) {
            auto result = device_.write_byte(static_cast<core::u8>(*str));
            if (result.is_error()) {
                return result;
            }
            ++str;
        }
        return core::Ok();
    }

   private:
    UartImpl device_;
};

}  // namespace alloy::hal

#endif  // ALLOY_HAL_INTERFACE_UART_HPP
