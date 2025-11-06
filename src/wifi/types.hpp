/**
 * @file types.hpp
 * @brief WiFi common types and structures
 *
 * Common types used across WiFi Station, AP, and scanning functionality.
 */

#pragma once

#include "core/types.hpp"
#include <array>
#include <stdint.h>

namespace alloy::wifi {

/**
 * @brief WiFi authentication mode
 */
enum class AuthMode : uint8_t {
    Open = 0,           ///< Open network (no password)
    WEP,                ///< WEP (deprecated, insecure)
    WPA_PSK,            ///< WPA with PSK
    WPA2_PSK,           ///< WPA2 with PSK (most common)
    WPA_WPA2_PSK,       ///< WPA/WPA2 mixed mode
    WPA2_ENTERPRISE,    ///< WPA2 Enterprise (802.1X)
    WPA3_PSK,           ///< WPA3 with PSK
    WPA2_WPA3_PSK,      ///< WPA2/WPA3 mixed mode
};

/**
 * @brief WiFi connection state
 */
enum class ConnectionState : uint8_t {
    Disconnected = 0,   ///< Not connected
    Connecting,         ///< Connection in progress
    Connected,          ///< Connected to AP
    GotIP,              ///< Connected and got IP address
    Failed,             ///< Connection failed
};

/**
 * @brief MAC address (6 bytes)
 */
struct MacAddress {
    std::array<uint8_t, 6> bytes;

    MacAddress() : bytes{0} {}

    explicit MacAddress(const uint8_t* mac) {
        for (int i = 0; i < 6; i++) {
            bytes[i] = mac[i];
        }
    }

    bool operator==(const MacAddress& other) const {
        return bytes == other.bytes;
    }

    bool operator!=(const MacAddress& other) const {
        return bytes != other.bytes;
    }

    /**
     * @brief Check if MAC address is zero (invalid)
     */
    bool is_zero() const {
        for (uint8_t b : bytes) {
            if (b != 0) return false;
        }
        return true;
    }
};

/**
 * @brief IPv4 address
 */
struct IPAddress {
    uint8_t octet[4];

    IPAddress() : octet{0, 0, 0, 0} {}

    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : octet{a, b, c, d} {}

    explicit IPAddress(uint32_t ip) {
        octet[0] = (ip >> 0) & 0xFF;
        octet[1] = (ip >> 8) & 0xFF;
        octet[2] = (ip >> 16) & 0xFF;
        octet[3] = (ip >> 24) & 0xFF;
    }

    uint32_t to_u32() const {
        return (uint32_t)octet[0]
             | ((uint32_t)octet[1] << 8)
             | ((uint32_t)octet[2] << 16)
             | ((uint32_t)octet[3] << 24);
    }

    bool operator==(const IPAddress& other) const {
        return octet[0] == other.octet[0]
            && octet[1] == other.octet[1]
            && octet[2] == other.octet[2]
            && octet[3] == other.octet[3];
    }

    bool operator!=(const IPAddress& other) const {
        return !(*this == other);
    }

    /**
     * @brief Check if IP address is zero (invalid)
     */
    bool is_zero() const {
        return octet[0] == 0 && octet[1] == 0 && octet[2] == 0 && octet[3] == 0;
    }
};

/**
 * @brief WiFi network information (from scan)
 */
struct AccessPointInfo {
    char ssid[33];          ///< SSID (max 32 chars + null)
    MacAddress bssid;       ///< MAC address of AP
    uint8_t channel;        ///< WiFi channel (1-14)
    int8_t rssi;            ///< Signal strength in dBm
    AuthMode auth_mode;     ///< Authentication mode

    AccessPointInfo()
        : ssid{0}
        , bssid()
        , channel(0)
        , rssi(0)
        , auth_mode(AuthMode::Open)
    {}
};

/**
 * @brief WiFi connection info (IP configuration)
 */
struct ConnectionInfo {
    IPAddress ip;           ///< Device IP address
    IPAddress gateway;      ///< Gateway IP address
    IPAddress netmask;      ///< Subnet mask
    MacAddress mac;         ///< Device MAC address

    ConnectionInfo()
        : ip(), gateway(), netmask(), mac()
    {}
};

/**
 * @brief Station information (for AP mode)
 */
struct StationInfo {
    MacAddress mac;         ///< Station MAC address
    int8_t rssi;            ///< Signal strength

    StationInfo() : mac(), rssi(0) {}
};

/**
 * @brief WiFi event types
 */
enum class WiFiEvent : uint8_t {
    StationStart,           ///< Station mode started
    StationStop,            ///< Station mode stopped
    StationConnected,       ///< Connected to AP
    StationDisconnected,    ///< Disconnected from AP
    StationGotIP,           ///< Got IP address
    APStart,                ///< AP mode started
    APStop,                 ///< AP mode stopped
    APStationConnected,     ///< Station connected to our AP
    APStationDisconnected,  ///< Station disconnected from our AP
    ScanDone,               ///< WiFi scan completed
};

} // namespace alloy::wifi
