#ifndef ALLOY_BLE_CENTRAL_HPP
#define ALLOY_BLE_CENTRAL_HPP

#include "core/error.hpp"

#include "types.hpp"

using alloy::core::u16;
using alloy::core::u8;

/// BLE Central (Client/Scanner) API for Alloy framework
///
/// Provides a C++ abstraction for BLE GATT client operations including:
/// - Device scanning and discovery
/// - Connection to BLE peripherals
/// - Service and characteristic discovery
/// - Characteristic read/write/subscribe operations
///
/// Uses RAII pattern for automatic resource cleanup.

namespace alloy::ble {

using alloy::core::ErrorCode;
using alloy::core::Result;

/// BLE scan configuration
struct ScanConfig {
    ScanType type;            // Scan type (passive/active)
    u16 interval;             // Scan interval (0.625ms units)
    u16 window;               // Scan window (0.625ms units)
    u16 duration;             // Scan duration (ms, 0 = forever)
    ScanFilterPolicy filter;  // Filter policy

    ScanConfig()
        : type(ScanType::Active),
          interval(80)  // 50ms
          ,
          window(48)  // 30ms
          ,
          duration(5000)  // 5s
          ,
          filter(ScanFilterPolicy::AcceptAll) {}
};

/// BLE Central (GATT Client/Scanner)
///
/// Manages BLE central operations including scanning for devices,
/// connecting to peripherals, and GATT client operations.
///
/// Example:
///   Central central;
///   central.init();
///   auto result = central.scan(5000);
///   if (result.is_ok()) {
///       auto devices = central.get_scan_results();
///       // Connect to device...
///   }
///
class Central {
   public:
    /// Constructor
    Central();

    /// Destructor - disconnects and cleans up
    ~Central();

    // Prevent copying
    Central(const Central&) = delete;
    Central& operator=(const Central&) = delete;

    // ========================================================================
    // Initialization
    // ========================================================================

    /// Initialize BLE central
    ///
    /// Initializes BT controller, Bluedroid stack, and GATTC.
    ///
    /// @return Result<void> - Ok on success, error on failure
    Result<void> init();

    /// Deinitialize BLE central
    ///
    /// Disconnects from peripherals and cleans up resources.
    ///
    /// @return Result<void> - Ok on success, error on failure
    Result<void> deinit();

    // ========================================================================
    // Scanning
    // ========================================================================

    /// Start scanning for BLE devices (blocking)
    ///
    /// Scans for specified duration and stores results.
    ///
    /// @param duration_ms Scan duration in milliseconds
    /// @return Result<u8> - Number of devices found on success
    Result<u8> scan(u16 duration_ms = 5000);

    /// Start scanning with custom configuration (blocking)
    ///
    /// @param config Scan configuration
    /// @return Result<u8> - Number of devices found on success
    Result<u8> scan(const ScanConfig& config);

    /// Start scanning asynchronously
    ///
    /// Scan results delivered via callback.
    ///
    /// @param config Scan configuration
    /// @return Result<void> - Ok if scan started successfully
    Result<void> scan_async(const ScanConfig& config);

    /// Stop active scan
    ///
    /// @return Result<void> - Ok on success, error on failure
    Result<void> stop_scan();

    /// Check if currently scanning
    ///
    /// @return true if scanning, false otherwise
    bool is_scanning() const;

    /// Get scan results
    ///
    /// Returns devices discovered in last scan.
    ///
    /// @param devices Buffer to store device info
    /// @param max_devices Maximum number of devices to return
    /// @return Result<u8> - Number of devices returned
    Result<u8> get_scan_results(DeviceInfo* devices, u8 max_devices) const;

    /// Clear scan results
    void clear_scan_results();

    // ========================================================================
    // Connection Management
    // ========================================================================

    /// Connect to BLE peripheral
    ///
    /// Establishes connection to specified device.
    ///
    /// @param address Device address
    /// @param timeout_ms Connection timeout in milliseconds
    /// @return Result<ConnHandle> - Connection handle on success
    Result<ConnHandle> connect(const Address& address, u16 timeout_ms = 10000);

    /// Connect to BLE peripheral with parameters
    ///
    /// @param address Device address
    /// @param params Connection parameters
    /// @param timeout_ms Connection timeout in milliseconds
    /// @return Result<ConnHandle> - Connection handle on success
    Result<ConnHandle> connect(const Address& address, const ConnParams& params,
                               u16 timeout_ms = 10000);

    /// Disconnect from peripheral
    ///
    /// @param conn_handle Connection handle
    /// @return Result<void> - Ok on success, error on failure
    Result<void> disconnect(ConnHandle conn_handle);

    /// Check if connected to peripheral
    ///
    /// @param conn_handle Connection handle
    /// @return true if connected, false otherwise
    bool is_connected(ConnHandle conn_handle) const;

    /// Get connection state
    ///
    /// @param conn_handle Connection handle
    /// @return Connection state
    ConnState get_conn_state(ConnHandle conn_handle) const;

    // ========================================================================
    // Service Discovery
    // ========================================================================

    /// Discover all services on peripheral
    ///
    /// @param conn_handle Connection handle
    /// @return Result<u8> - Number of services discovered
    Result<u8> discover_services(ConnHandle conn_handle);

    /// Discover service by UUID
    ///
    /// @param conn_handle Connection handle
    /// @param service_uuid Service UUID to find
    /// @return Result<ServiceHandle> - Service handle on success
    Result<ServiceHandle> discover_service(ConnHandle conn_handle, const UUID& service_uuid);

    /// Get discovered services
    ///
    /// @param conn_handle Connection handle
    /// @param services Buffer to store service handles
    /// @param max_services Maximum number of services
    /// @return Result<u8> - Number of services returned
    Result<u8> get_services(ConnHandle conn_handle, ServiceHandle* services, u8 max_services) const;

    // ========================================================================
    // Characteristic Discovery
    // ========================================================================

    /// Discover all characteristics in service
    ///
    /// @param conn_handle Connection handle
    /// @param service Service handle
    /// @return Result<u8> - Number of characteristics discovered
    Result<u8> discover_characteristics(ConnHandle conn_handle, const ServiceHandle& service);

    /// Discover characteristic by UUID
    ///
    /// @param conn_handle Connection handle
    /// @param service Service handle
    /// @param char_uuid Characteristic UUID
    /// @return Result<CharHandle> - Characteristic handle on success
    Result<CharHandle> discover_characteristic(ConnHandle conn_handle, const ServiceHandle& service,
                                               const UUID& char_uuid);

    /// Get discovered characteristics
    ///
    /// @param conn_handle Connection handle
    /// @param service Service handle
    /// @param characteristics Buffer to store characteristic handles
    /// @param max_chars Maximum number of characteristics
    /// @return Result<u8> - Number of characteristics returned
    Result<u8> get_characteristics(ConnHandle conn_handle, const ServiceHandle& service,
                                   CharHandle* characteristics, u8 max_chars) const;

    // ========================================================================
    // Characteristic Operations
    // ========================================================================

    /// Read characteristic value
    ///
    /// @param conn_handle Connection handle
    /// @param characteristic Characteristic handle
    /// @param buffer Buffer to store value
    /// @param buffer_size Size of buffer
    /// @return Result<u16> - Number of bytes read on success
    Result<u16> read_char(ConnHandle conn_handle, const CharHandle& characteristic, u8* buffer,
                          u16 buffer_size);

    /// Write characteristic value
    ///
    /// @param conn_handle Connection handle
    /// @param characteristic Characteristic handle
    /// @param data Data to write
    /// @param length Data length
    /// @return Result<void> - Ok on success, error on failure
    Result<void> write_char(ConnHandle conn_handle, const CharHandle& characteristic,
                            const u8* data, u16 length);

    /// Write characteristic without response
    ///
    /// @param conn_handle Connection handle
    /// @param characteristic Characteristic handle
    /// @param data Data to write
    /// @param length Data length
    /// @return Result<void> - Ok on success, error on failure
    Result<void> write_char_no_response(ConnHandle conn_handle, const CharHandle& characteristic,
                                        const u8* data, u16 length);

    /// Subscribe to characteristic notifications
    ///
    /// @param conn_handle Connection handle
    /// @param characteristic Characteristic handle
    /// @return Result<void> - Ok on success, error on failure
    Result<void> subscribe(ConnHandle conn_handle, const CharHandle& characteristic);

    /// Unsubscribe from characteristic notifications
    ///
    /// @param conn_handle Connection handle
    /// @param characteristic Characteristic handle
    /// @return Result<void> - Ok on success, error on failure
    Result<void> unsubscribe(ConnHandle conn_handle, const CharHandle& characteristic);

    // ========================================================================
    // Callbacks
    // ========================================================================

    /// Set scan callback
    ///
    /// Called for each device discovered during scanning.
    ///
    /// @param callback Scan callback function
    void set_scan_callback(ScanCallback callback);

    /// Set connection callback
    ///
    /// Called when connection state changes.
    ///
    /// @param callback Connection callback function
    void set_connection_callback(ConnectionCallback callback);

    /// Set characteristic read callback
    ///
    /// Called when characteristic read completes.
    ///
    /// @param callback Read callback function
    void set_read_callback(ReadCallback callback);

    /// Set characteristic notification callback
    ///
    /// Called when notification/indication is received.
    ///
    /// @param callback Notification callback function
    void set_notify_callback(WriteCallback callback);

   private:
    struct Impl;
    Impl* impl_;
};

}  // namespace alloy::ble

#endif  // ALLOY_BLE_CENTRAL_HPP
