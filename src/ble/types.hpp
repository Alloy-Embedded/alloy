#ifndef ALLOY_BLE_TYPES_HPP
#define ALLOY_BLE_TYPES_HPP

#include "core/types.hpp"
#include <cstring>

using alloy::core::u8;
using alloy::core::u16;
using alloy::core::u32;
using alloy::core::i8;
using alloy::core::i16;
using alloy::core::i32;

/// BLE (Bluetooth Low Energy) types for Alloy framework
///
/// Provides common types and structures for BLE communication including
/// device addresses, UUIDs, advertisement data, and GATT attributes.

namespace alloy::ble {

// ============================================================================
// Core BLE Types
// ============================================================================

/// BLE device address (6 bytes)
struct Address {
    u8 addr[6];

    Address() {
        memset(addr, 0, 6);
    }

    Address(u8 a0, u8 a1, u8 a2, u8 a3, u8 a4, u8 a5) {
        addr[0] = a0; addr[1] = a1; addr[2] = a2;
        addr[3] = a3; addr[4] = a4; addr[5] = a5;
    }

    Address(const u8* data) {
        memcpy(addr, data, 6);
    }

    bool operator==(const Address& other) const {
        return memcmp(addr, other.addr, 6) == 0;
    }

    bool operator!=(const Address& other) const {
        return !(*this == other);
    }
};

/// BLE address type
enum class AddressType : u8 {
    Public = 0,      // Public device address
    Random = 1,      // Random device address
    PublicID = 2,    // Public identity address (resolved from RPA)
    RandomID = 3,    // Random identity address (resolved from RPA)
};

/// BLE UUID (16-bit, 32-bit, or 128-bit)
struct UUID {
    enum class Type : u8 {
        UUID16 = 16,
        UUID32 = 32,
        UUID128 = 128,
    };

    Type type;
    union {
        u16 uuid16;
        u32 uuid32;
        u8 uuid128[16];
    };

    UUID() : type(Type::UUID16), uuid16(0) {}

    explicit UUID(u16 uuid) : type(Type::UUID16), uuid16(uuid) {}

    explicit UUID(u32 uuid) : type(Type::UUID32), uuid32(uuid) {}

    explicit UUID(const u8* uuid) : type(Type::UUID128) {
        memcpy(uuid128, uuid, 16);
    }

    bool operator==(const UUID& other) const {
        if (type != other.type) return false;
        switch (type) {
            case Type::UUID16:
                return uuid16 == other.uuid16;
            case Type::UUID32:
                return uuid32 == other.uuid32;
            case Type::UUID128:
                return memcmp(uuid128, other.uuid128, 16) == 0;
        }
        return false;
    }

    bool operator!=(const UUID& other) const {
        return !(*this == other);
    }
};

// ============================================================================
// Advertisement Types
// ============================================================================

/// BLE advertisement type
enum class AdvType : u8 {
    ConnectableUndirected = 0,    // ADV_IND
    ConnectableDirected = 1,      // ADV_DIRECT_IND
    ScannableUndirected = 2,      // ADV_SCAN_IND
    NonConnectable = 3,           // ADV_NONCONN_IND
    ScanResponse = 4,             // SCAN_RSP
};

/// Advertisement data
struct AdvData {
    char name[32];              // Device name
    u16 appearance;             // Appearance value
    bool include_name;          // Include name in advertisement
    bool include_txpower;       // Include TX power in advertisement
    u8 flags;                   // Advertisement flags

    AdvData()
        : appearance(0)
        , include_name(true)
        , include_txpower(false)
        , flags(0x06) // General discoverable + BR/EDR not supported
    {
        name[0] = '\0';
    }
};

/// Scan response data
struct ScanRspData {
    UUID* service_uuids;        // Array of service UUIDs
    u8 service_count;           // Number of service UUIDs
    u8* manufacturer_data;      // Manufacturer specific data
    u16 manufacturer_data_len;  // Length of manufacturer data

    ScanRspData()
        : service_uuids(nullptr)
        , service_count(0)
        , manufacturer_data(nullptr)
        , manufacturer_data_len(0)
    {}
};

// ============================================================================
// Scan Types
// ============================================================================

/// BLE scan type
enum class ScanType : u8 {
    Passive = 0,    // Passive scanning (no scan requests)
    Active = 1,     // Active scanning (send scan requests)
};

/// BLE scan filter policy
enum class ScanFilterPolicy : u8 {
    AcceptAll = 0,              // Accept all advertisements
    WhiteListOnly = 1,          // Accept only from white list
    AcceptAllResolvable = 2,    // Accept all + use IRK for directed
    WhiteListResolvable = 3,    // White list + use IRK for directed
};

/// Discovered BLE device information
struct DeviceInfo {
    Address address;            // Device address
    AddressType addr_type;      // Address type
    i8 rssi;                    // Signal strength (dBm)
    AdvType adv_type;           // Advertisement type
    char name[32];              // Device name (if available)
    u8 adv_data[31];           // Raw advertisement data
    u8 adv_data_len;           // Advertisement data length

    DeviceInfo()
        : addr_type(AddressType::Public)
        , rssi(0)
        , adv_type(AdvType::ConnectableUndirected)
        , adv_data_len(0)
    {
        name[0] = '\0';
        memset(adv_data, 0, 31);
    }
};

// ============================================================================
// GATT Types
// ============================================================================

/// GATT characteristic properties
enum class CharProperty : u8 {
    Broadcast = 0x01,           // Broadcast
    Read = 0x02,                // Read
    WriteNoResponse = 0x04,     // Write without response
    Write = 0x08,               // Write
    Notify = 0x10,              // Notify
    Indicate = 0x20,            // Indicate
    AuthWrite = 0x40,           // Authenticated write
    ExtProp = 0x80,             // Extended properties
};

/// GATT characteristic permissions
enum class CharPermission : u8 {
    Read = 0x01,                // Read
    Write = 0x02,               // Write
    ReadEncrypted = 0x04,       // Encrypted read
    WriteEncrypted = 0x08,      // Encrypted write
};

/// GATT attribute handle
using AttrHandle = u16;

/// GATT connection handle
using ConnHandle = u16;

/// Invalid handles
constexpr AttrHandle INVALID_ATTR_HANDLE = 0;
constexpr ConnHandle INVALID_CONN_HANDLE = 0xFFFF;

/// Forward declaration
struct UUID;

/// GATT Service handle
struct ServiceHandle {
    AttrHandle handle;
    UUID uuid;

    ServiceHandle();
    ServiceHandle(AttrHandle h, const UUID& u);
    bool is_valid() const;
};

/// GATT Characteristic handle
struct CharHandle {
    AttrHandle handle;
    UUID uuid;
    u8 properties;

    CharHandle();
    CharHandle(AttrHandle h, const UUID& u, u8 props);
    bool is_valid() const;
};

/// GATT characteristic value
struct CharValue {
    u8* data;                   // Value data
    u16 length;                 // Value length
    u16 max_length;             // Maximum value length

    CharValue()
        : data(nullptr)
        , length(0)
        , max_length(0)
    {}

    CharValue(u8* buf, u16 len, u16 max_len)
        : data(buf)
        , length(len)
        , max_length(max_len)
    {}
};

// ============================================================================
// Connection Types
// ============================================================================

/// BLE connection parameters
struct ConnParams {
    u16 interval_min;           // Min connection interval (units of 1.25ms)
    u16 interval_max;           // Max connection interval (units of 1.25ms)
    u16 latency;                // Slave latency (connection events)
    u16 timeout;                // Supervision timeout (units of 10ms)

    ConnParams()
        : interval_min(80)      // 100ms
        , interval_max(100)     // 125ms
        , latency(0)
        , timeout(400)          // 4s
    {}
};

/// Connection state
enum class ConnState : u8 {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Disconnecting = 3,
};

/// Disconnection reason
enum class DisconnectReason : u8 {
    Success = 0x00,
    RemoteUserTerminated = 0x13,
    RemoteLowResources = 0x14,
    RemotePowerOff = 0x15,
    LocalTerminated = 0x16,
    Timeout = 0x08,
    Unknown = 0xFF,
};

// ============================================================================
// Callback Types
// ============================================================================

/// Connection callback signature: void (*)(ConnHandle handle, bool connected)
using ConnectionCallback = void (*)(ConnHandle handle, bool connected);

/// Scan result callback signature: void (*)(const DeviceInfo& device)
using ScanCallback = void (*)(const DeviceInfo& device);

/// Characteristic read callback signature: void (*)(AttrHandle handle, const CharValue& value)
using ReadCallback = void (*)(AttrHandle handle, const CharValue& value);

/// Characteristic write callback signature: void (*)(AttrHandle handle, const CharValue& value)
using WriteCallback = void (*)(AttrHandle handle, const CharValue& value);

// ============================================================================
// Inline Implementations
// ============================================================================

inline ServiceHandle::ServiceHandle() : handle(INVALID_ATTR_HANDLE), uuid() {}

inline ServiceHandle::ServiceHandle(AttrHandle h, const UUID& u) : handle(h), uuid(u) {}

inline bool ServiceHandle::is_valid() const { return handle != INVALID_ATTR_HANDLE; }

inline CharHandle::CharHandle() : handle(INVALID_ATTR_HANDLE), uuid(), properties(0) {}

inline CharHandle::CharHandle(AttrHandle h, const UUID& u, u8 props)
    : handle(h), uuid(u), properties(props) {}

inline bool CharHandle::is_valid() const { return handle != INVALID_ATTR_HANDLE; }

} // namespace alloy::ble

#endif // ALLOY_BLE_TYPES_HPP
