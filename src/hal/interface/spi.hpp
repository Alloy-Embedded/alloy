/// Platform-agnostic SPI interface
///
/// Defines SPI concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_SPI_HPP
#define ALLOY_HAL_INTERFACE_SPI_HPP

#include <concepts>
#include <span>

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/// SPI clock mode (CPOL/CPHA)
///
/// Defines the clock polarity and phase configuration.
/// - CPOL (Clock Polarity): idle state of clock (0 = low, 1 = high)
/// - CPHA (Clock Phase): data capture edge (0 = first, 1 = second)
enum class SpiMode : u8 {
    Mode0 = 0,  ///< CPOL=0, CPHA=0 (sample on rising edge)
    Mode1 = 1,  ///< CPOL=0, CPHA=1 (sample on falling edge)
    Mode2 = 2,  ///< CPOL=1, CPHA=0 (sample on falling edge)
    Mode3 = 3   ///< CPOL=1, CPHA=1 (sample on rising edge)
};

/// SPI bit order
enum class SpiBitOrder : u8 {
    MsbFirst = 0,  ///< Most significant bit first (standard)
    LsbFirst = 1   ///< Least significant bit first
};

/// SPI data size
enum class SpiDataSize : u8 {
    Bits8 = 8,   ///< 8-bit data frames
    Bits16 = 16  ///< 16-bit data frames
};

/// SPI configuration parameters
///
/// Contains all parameters needed to configure a SPI peripheral.
struct SpiConfig {
    SpiMode mode;
    u32 clock_speed;  ///< Clock speed in Hz
    SpiBitOrder bit_order;
    SpiDataSize data_size;

    /// Constructor with default configuration
    constexpr SpiConfig(SpiMode m = SpiMode::Mode0,
                        u32 speed = 1000000,  // 1 MHz default
                        SpiBitOrder order = SpiBitOrder::MsbFirst,
                        SpiDataSize size = SpiDataSize::Bits8)
        : mode(m),
          clock_speed(speed),
          bit_order(order),
          data_size(size) {}
};

/// SPI master device concept
///
/// Defines the interface that all SPI master implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
///
/// Note: Chip select (CS) management is typically done via GPIO pins,
/// which should be controlled separately by the application.
template <typename T>
concept SpiMaster = requires(T device, const T const_device, std::span<u8> buffer,
                             std::span<const u8> const_buffer, SpiConfig config) {
    /// Transfer data (full duplex)
    ///
    /// Simultaneously sends and receives data. The tx_buffer and rx_buffer
    /// can be different sizes - the actual transfer length is the minimum
    /// of both.
    ///
    /// @param tx_buffer Buffer containing data to transmit
    /// @param rx_buffer Buffer to store received data
    /// @return Ok on success, error code on failure
    { device.transfer(const_buffer, buffer) } -> std::same_as<Result<void, ErrorCode>>;

    /// Transmit only (simplex)
    ///
    /// Sends data without receiving. More efficient when receive data
    /// is not needed.
    ///
    /// @param tx_buffer Buffer containing data to transmit
    /// @return Ok on success, error code on failure
    { device.transmit(const_buffer) } -> std::same_as<Result<void, ErrorCode>>;

    /// Receive only (simplex)
    ///
    /// Receives data without meaningful transmission (sends dummy bytes).
    ///
    /// @param rx_buffer Buffer to store received data
    /// @return Ok on success, error code on failure
    { device.receive(buffer) } -> std::same_as<Result<void, ErrorCode>>;

    /// Configure SPI peripheral
    ///
    /// @param config SPI configuration (mode, speed, bit order, data size)
    /// @return Ok on success, error code on failure
    { device.configure(config) } -> std::same_as<Result<void, ErrorCode>>;

    /// Check if SPI is busy (transfer in progress)
    ///
    /// @return true if busy, false if ready
    { const_device.is_busy() } -> std::same_as<bool>;
};

/// Helper function to transfer a single byte via SPI
///
/// @tparam Device SPI device type satisfying SpiMaster concept
/// @param device SPI device instance
/// @param tx_byte Byte to transmit
/// @return Received byte or error code
template <SpiMaster Device>
Result<u8, ErrorCode> spi_transfer_byte(Device& device, u8 tx_byte) {
    u8 rx_byte = 0;
    auto tx_buf = std::span(&tx_byte, 1);
    auto rx_buf = std::span(&rx_byte, 1);

    auto result = device.transfer(tx_buf, rx_buf);

    if (!result.is_ok()) {
        return Err(std::move(result).error());
    }

    return Ok(static_cast<u8>(rx_byte));
}

/// Helper function to write a single byte via SPI
///
/// @tparam Device SPI device type satisfying SpiMaster concept
/// @param device SPI device instance
/// @param byte Byte to write
/// @return Ok on success, error code on failure
template <SpiMaster Device>
Result<void, ErrorCode> spi_write_byte(Device& device, u8 byte) {
    auto buffer = std::span(&byte, 1);
    return device.transmit(buffer);
}

/// Helper function to read a single byte via SPI
///
/// @tparam Device SPI device type satisfying SpiMaster concept
/// @param device SPI device instance
/// @return Received byte or error code
template <SpiMaster Device>
Result<u8, ErrorCode> spi_read_byte(Device& device) {
    u8 byte = 0;
    auto buffer = std::span(&byte, 1);

    auto result = device.receive(buffer);

    if (!result.is_ok()) {
        return Err(std::move(result).error());
    }

    return Ok(static_cast<u8>(byte));
}

/// RAII helper for SPI chip select
///
/// Automatically manages chip select pin during SPI transaction.
/// Selects on construction, deselects on destruction.
///
/// Example usage:
/// \code
/// {
///     SpiChipSelect cs(cs_pin);  // CS goes low
///     spi.transmit(data);
/// }  // CS goes high automatically
/// \endcode
template <typename GpioPin>
class SpiChipSelect {
   public:
    /// Constructor - asserts chip select (active low)
    explicit SpiChipSelect(GpioPin& pin) : pin_(pin) { pin_.set_low(); }

    /// Destructor - deasserts chip select
    ~SpiChipSelect() { pin_.set_high(); }

    // Disable copy and move
    SpiChipSelect(const SpiChipSelect&) = delete;
    SpiChipSelect& operator=(const SpiChipSelect&) = delete;
    SpiChipSelect(SpiChipSelect&&) = delete;
    SpiChipSelect& operator=(SpiChipSelect&&) = delete;

   private:
    GpioPin& pin_;
};

}  // namespace alloy::hal

#endif  // ALLOY_HAL_INTERFACE_SPI_HPP
