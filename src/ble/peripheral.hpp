#ifndef ALLOY_BLE_PERIPHERAL_HPP
#define ALLOY_BLE_PERIPHERAL_HPP

#include "core/error.hpp"

#include "types.hpp"

using alloy::core::u16;
using alloy::core::u8;

/// BLE Peripheral (Server) API for Alloy framework
///
/// Provides a C++ abstraction for BLE GATT server operations including:
/// - Advertisement and discoverability
/// - GATT service and characteristic registration
/// - Characteristic read/write/notify operations
/// - Connection management
///
/// Uses RAII pattern for automatic resource cleanup.

namespace alloy::ble {

using alloy::core::ErrorCode;
using alloy::core::Result;

/// BLE Peripheral configuration
struct PeripheralConfig {
    char device_name[32];  // BLE device name
    u16 appearance;        // Appearance value
    u16 min_interval;      // Min connection interval (1.25ms units)
    u16 max_interval;      // Max connection interval (1.25ms units)
    u16 latency;           // Slave latency
    u16 timeout;           // Supervision timeout (10ms units)

    PeripheralConfig()
        : appearance(0),
          min_interval(80)  // 100ms
          ,
          max_interval(100)  // 125ms
          ,
          latency(0),
          timeout(400)  // 4s
    {
        strcpy(device_name, "Alloy Device");
    }
};

/// BLE Peripheral (GATT Server)
///
/// Manages BLE peripheral operations including advertising, GATT services,
/// and characteristic operations. Uses RAII pattern for cleanup.
///
/// Example:
///   Peripheral peripheral;
///   peripheral.init("My Device");
///   peripheral.add_service(UUID(0x1800));  // Generic Access
///   peripheral.start_advertising();
///
class Peripheral {
   public:
    /// Constructor
    Peripheral();

    /// Destructor - stops advertising and cleans up
    ~Peripheral();

    // Prevent copying
    Peripheral(const Peripheral&) = delete;
    Peripheral& operator=(const Peripheral&) = delete;

    // ========================================================================
    // Initialization
    // ========================================================================

    /// Initialize BLE peripheral with default configuration
    ///
    /// Initializes BT controller, Bluedroid stack, and GATTS.
    ///
    /// @param device_name Device name to advertise
    /// @return Result<void> - Ok on success, error on failure
    Result<void> init(const char* device_name = "Alloy Device");

    /// Initialize BLE peripheral with custom configuration
    ///
    /// @param config Peripheral configuration
    /// @return Result<void> - Ok on success, error on failure
    Result<void> init(const PeripheralConfig& config);

    /// Deinitialize BLE peripheral
    ///
    /// Stops advertising, disconnects clients, and cleans up resources.
    ///
    /// @return Result<void> - Ok on success, error on failure
    Result<void> deinit();

    // ========================================================================
    // Advertisement
    // ========================================================================

    /// Start advertising
    ///
    /// Begins BLE advertising with configured name and services.
    ///
    /// @return Result<void> - Ok on success, error on failure
    Result<void> start_advertising();

    /// Start advertising with custom data
    ///
    /// @param adv_data Advertisement data
    /// @return Result<void> - Ok on success, error on failure
    Result<void> start_advertising(const AdvData& adv_data);

    /// Stop advertising
    ///
    /// @return Result<void> - Ok on success, error on failure
    Result<void> stop_advertising();

    /// Check if currently advertising
    ///
    /// @return true if advertising, false otherwise
    bool is_advertising() const;

    // ========================================================================
    // GATT Service Management
    // ========================================================================

    /// Add GATT service
    ///
    /// Creates a new GATT service with the specified UUID.
    ///
    /// @param uuid Service UUID
    /// @return Result<ServiceHandle> - Service handle on success
    Result<ServiceHandle> add_service(const UUID& uuid);

    /// Start GATT service
    ///
    /// Must be called after adding all characteristics to a service.
    ///
    /// @param service Service handle
    /// @return Result<void> - Ok on success, error on failure
    Result<void> start_service(const ServiceHandle& service);

    // ========================================================================
    // GATT Characteristic Management
    // ========================================================================

    /// Add characteristic to service
    ///
    /// Creates a new GATT characteristic with specified properties.
    ///
    /// @param service Service handle
    /// @param uuid Characteristic UUID
    /// @param properties Characteristic properties (bitmask of CharProperty)
    /// @param permissions Characteristic permissions (bitmask of CharPermission)
    /// @return Result<CharHandle> - Characteristic handle on success
    Result<CharHandle> add_characteristic(const ServiceHandle& service, const UUID& uuid,
                                          u8 properties,
                                          u8 permissions = static_cast<u8>(CharPermission::Read) |
                                                           static_cast<u8>(CharPermission::Write));

    /// Set characteristic value
    ///
    /// Updates the stored value of a characteristic.
    ///
    /// @param characteristic Characteristic handle
    /// @param data Value data
    /// @param length Value length
    /// @return Result<void> - Ok on success, error on failure
    Result<void> set_char_value(const CharHandle& characteristic, const u8* data, u16 length);

    /// Get characteristic value
    ///
    /// Retrieves the current stored value of a characteristic.
    ///
    /// @param characteristic Characteristic handle
    /// @param buffer Buffer to store value
    /// @param buffer_size Size of buffer
    /// @return Result<u16> - Number of bytes read on success
    Result<u16> get_char_value(const CharHandle& characteristic, u8* buffer, u16 buffer_size) const;

    /// Send characteristic notification
    ///
    /// Sends a notification to connected clients. Characteristic must have
    /// Notify property enabled.
    ///
    /// @param characteristic Characteristic handle
    /// @param data Notification data
    /// @param length Data length
    /// @return Result<void> - Ok on success, error on failure
    Result<void> notify(const CharHandle& characteristic, const u8* data, u16 length);

    /// Send characteristic indication
    ///
    /// Sends an indication to connected clients (requires acknowledgment).
    /// Characteristic must have Indicate property enabled.
    ///
    /// @param characteristic Characteristic handle
    /// @param data Indication data
    /// @param length Data length
    /// @return Result<void> - Ok on success, error on failure
    Result<void> indicate(const CharHandle& characteristic, const u8* data, u16 length);

    // ========================================================================
    // Connection Management
    // ========================================================================

    /// Check if any client is connected
    ///
    /// @return true if connected, false otherwise
    bool is_connected() const;

    /// Get number of connected clients
    ///
    /// @return Number of connected clients
    u8 connection_count() const;

    /// Disconnect all clients
    ///
    /// @return Result<void> - Ok on success, error on failure
    Result<void> disconnect_all();

    /// Disconnect specific client
    ///
    /// @param conn_handle Connection handle
    /// @return Result<void> - Ok on success, error on failure
    Result<void> disconnect(ConnHandle conn_handle);

    // ========================================================================
    // Callbacks
    // ========================================================================

    /// Set connection callback
    ///
    /// Called when client connects or disconnects.
    ///
    /// @param callback Connection callback function
    void set_connection_callback(ConnectionCallback callback);

    /// Set characteristic read callback
    ///
    /// Called when client reads a characteristic.
    ///
    /// @param callback Read callback function
    void set_read_callback(ReadCallback callback);

    /// Set characteristic write callback
    ///
    /// Called when client writes to a characteristic.
    ///
    /// @param callback Write callback function
    void set_write_callback(WriteCallback callback);

   private:
    struct Impl;
    Impl* impl_;
};

}  // namespace alloy::ble

#endif  // ALLOY_BLE_PERIPHERAL_HPP
