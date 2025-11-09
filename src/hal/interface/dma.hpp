/// Platform-agnostic DMA (Direct Memory Access) interface
///
/// Defines DMA concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_DMA_HPP
#define ALLOY_HAL_INTERFACE_DMA_HPP

#include <concepts>
#include <functional>
#include <span>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

/// DMA transfer direction
enum class DmaDirection : core::u8 {
    MemoryToMemory,      ///< Memory to memory (fast array copy)
    MemoryToPeripheral,  ///< Memory to peripheral (e.g., UART TX, DAC)
    PeripheralToMemory   ///< Peripheral to memory (e.g., ADC, UART RX)
};

/// DMA transfer mode
enum class DmaMode : core::u8 {
    Normal,   ///< Single transfer, stops when complete
    Circular  ///< Circular buffer, wraps automatically
};

/// DMA channel priority
enum class DmaPriority : core::u8 {
    Low = 0,      ///< Low priority
    Medium = 1,   ///< Medium priority
    High = 2,     ///< High priority
    VeryHigh = 3  ///< Very high priority
};

/// DMA data width
enum class DmaDataWidth : core::u8 {
    Bits8 = 1,   ///< 8-bit transfers (byte)
    Bits16 = 2,  ///< 16-bit transfers (half-word)
    Bits32 = 4   ///< 32-bit transfers (word)
};

/// DMA configuration parameters
///
/// Contains all parameters needed to configure a DMA channel.
struct DmaConfig {
    DmaDirection direction;             ///< Transfer direction
    DmaMode mode;                       ///< Transfer mode (normal or circular)
    DmaPriority priority;               ///< Channel priority
    DmaDataWidth data_width;            ///< Transfer data width
    volatile void* peripheral_address;  ///< Peripheral address (for periph transfers)

    /// Constructor with default configuration
    constexpr DmaConfig(DmaDirection dir = DmaDirection::MemoryToMemory,
                        DmaMode m = DmaMode::Normal, DmaPriority prio = DmaPriority::Low,
                        DmaDataWidth width = DmaDataWidth::Bits8,
                        volatile void* periph_addr = nullptr)
        : direction(dir),
          mode(m),
          priority(prio),
          data_width(width),
          peripheral_address(periph_addr) {}
};

/// DMA channel concept
///
/// Defines the interface that all DMA implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
///
/// Error codes specific to DMA:
/// - ErrorCode::DmaTransferError: DMA transfer error occurred
/// - ErrorCode::DmaAlignmentError: Address alignment error
/// - ErrorCode::DmaChannelBusy: Channel already in use
/// - ErrorCode::InvalidParameter: Invalid address or size
template <typename T>
concept DmaChannel =
    requires(T device, const T const_device, DmaConfig config, const void* source,
             void* destination, core::usize size, std::function<void()> complete_callback,
             std::function<void(core::ErrorCode)> error_callback) {
        /// Start DMA transfer
        ///
        /// Initiates transfer from source to destination.
        /// For memory-to-peripheral: source is memory, destination is nullptr (uses
        /// config.peripheral_address) For peripheral-to-memory: source is nullptr (uses
        /// config.peripheral_address), destination is memory For memory-to-memory: both source and
        /// destination are valid memory addresses
        ///
        /// @param source Source address (or nullptr for peripheral source)
        /// @param destination Destination address (or nullptr for peripheral destination)
        /// @param size Number of data elements to transfer
        /// @return Ok on success, error code on failure
        { device.start_transfer(source, destination, size) } -> std::same_as<core::Result<void>>;

        /// Stop DMA transfer
        ///
        /// Aborts ongoing transfer immediately.
        ///
        /// @return Ok on success, error code on failure
        { device.stop_transfer() } -> std::same_as<core::Result<void>>;

        /// Check if transfer is complete
        ///
        /// @return true if transfer finished, false if still in progress
        { const_device.is_complete() } -> std::same_as<bool>;

        /// Check if transfer error occurred
        ///
        /// @return true if error occurred, false if no error
        { const_device.is_error() } -> std::same_as<bool>;

        /// Get remaining transfer count
        ///
        /// Returns number of data elements still to be transferred.
        ///
        /// @return Remaining count
        { const_device.get_remaining_count() } -> std::same_as<core::u32>;

        /// Set transfer complete callback
        ///
        /// Sets function to be called when transfer completes successfully.
        /// Callback executes in interrupt context.
        ///
        /// @param callback Function to call on completion
        /// @return Ok on success, error code on failure
        { device.set_complete_callback(complete_callback) } -> std::same_as<core::Result<void>>;

        /// Set transfer error callback
        ///
        /// Sets function to be called when transfer error occurs.
        /// Callback executes in interrupt context.
        ///
        /// @param callback Function to call on error
        /// @return Ok on success, error code on failure
        { device.set_error_callback(error_callback) } -> std::same_as<core::Result<void>>;

        /// Configure DMA channel
        ///
        /// @param config DMA configuration (direction, mode, priority, etc)
        /// @return Ok on success, error code on failure
        { device.configure(config) } -> std::same_as<core::Result<void>>;
    };

/// Helper function to check if address is aligned for DMA
///
/// DMA requires proper address alignment based on data width.
///
/// @param address Memory address to check
/// @param data_width DMA data width
/// @return true if aligned, false otherwise
inline constexpr bool is_dma_aligned(const void* address, DmaDataWidth data_width) {
    auto addr = reinterpret_cast<core::usize>(address);
    auto alignment = static_cast<core::u8>(data_width);
    return (addr % alignment) == 0;
}

/// Helper function to calculate DMA transfer size in bytes
///
/// @param element_count Number of elements
/// @param data_width Width of each element
/// @return Total size in bytes
inline constexpr core::usize calculate_dma_size(core::usize element_count,
                                                DmaDataWidth data_width) {
    return element_count * static_cast<core::u8>(data_width);
}

/// Helper function to get recommended DMA data width for a type
///
/// @tparam T Type of data to transfer
/// @return Recommended DMA data width
template <typename DataType>
inline constexpr DmaDataWidth get_dma_width_for_type() {
    if constexpr (sizeof(DataType) == 1) {
        return DmaDataWidth::Bits8;
    } else if constexpr (sizeof(DataType) == 2) {
        return DmaDataWidth::Bits16;
    } else if constexpr (sizeof(DataType) == 4) {
        return DmaDataWidth::Bits32;
    } else {
        // For larger types, use word transfers
        return DmaDataWidth::Bits32;
    }
}

}  // namespace alloy::hal

#endif  // ALLOY_HAL_INTERFACE_DMA_HPP
