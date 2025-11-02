# ESP32 WiFi Access Point Example

This example demonstrates how to use Alloy's WiFi Access Point abstraction to turn the ESP32 into a WiFi access point that other devices can connect to.

## Features

- **Simple AP Configuration**: Easy-to-use C++ API for creating a WiFi access point
- **Station Monitoring**: Track when devices connect and disconnect
- **Station Information**: Get MAC addresses and signal strength (RSSI) of connected devices
- **Event Callbacks**: Receive notifications when stations connect/disconnect
- **Result<T> Error Handling**: Type-safe error handling without exceptions

## Hardware Required

- ESP32 development board (any variant with WiFi support)
- USB cable for programming and power

## Configuration

The AP is configured in `main/main.cpp`:

```cpp
#define AP_SSID         "Alloy_AP"      // Network name
#define AP_PASSWORD     "alloy12345"       // Password (min 8 chars)
#define AP_CHANNEL      1                   // WiFi channel (1-13)
#define AP_MAX_CONN     4                   // Max simultaneous connections
```

You can customize these values before building.

## Building and Flashing

### Using ESP-IDF

```bash
# Configure (first time only)
idf.py set-target esp32
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

### Using build script (if available)

```bash
chmod +x build.sh
./build.sh
```

## Expected Output

After flashing, you should see output like:

```
I (xxx) wifi_ap: ========================================
I (xxx) wifi_ap:   Alloy WiFi Access Point Example
I (xxx) wifi_ap:   Using Alloy WiFi API
I (xxx) wifi_ap: ========================================
I (xxx) wifi_ap:
I (xxx) wifi_ap: Initializing WiFi Access Point...
I (xxx) wifi_ap: ✓ WiFi AP initialized
I (xxx) wifi_ap:
I (xxx) wifi_ap: Starting Access Point...
I (xxx) wifi_ap: SSID:     Alloy_AP
I (xxx) wifi_ap: Password: alloy12345
I (xxx) wifi_ap: Channel:  1
I (xxx) wifi_ap: Max Conn: 4
I (xxx) wifi_ap:
I (xxx) wifi_ap: ✓ Access Point started successfully!
I (xxx) wifi_ap:
I (xxx) wifi_ap: === Access Point Information ===
I (xxx) wifi_ap: SSID:     Alloy_AP
I (xxx) wifi_ap: Password: alloy12345
I (xxx) wifi_ap: Channel:  1
I (xxx) wifi_ap: IP:       192.168.4.1
I (xxx) wifi_ap: Gateway:  192.168.4.1
I (xxx) wifi_ap: MAC:      XX:XX:XX:XX:XX:XX
I (xxx) wifi_ap: ================================
```

When a device connects:

```
I (xxx) wifi_ap:
I (xxx) wifi_ap: ✓ Station Connected!
I (xxx) wifi_ap:   MAC: XX:XX:XX:XX:XX:XX
I (xxx) wifi_ap:   Total stations: 1
```

## Testing

1. **Start the AP**: Flash and run the example
2. **Connect from another device**:
   - On your phone/laptop, look for WiFi network "Alloy_AP"
   - Connect using password "alloy12345"
   - Your device should receive IP 192.168.4.2 (or .3, .4, etc.)
3. **Verify connectivity**:
   - Ping the ESP32: `ping 192.168.4.1`
   - Watch the serial monitor for connection events
4. **Disconnect**: Turn off WiFi on your device and watch disconnect event

## How It Works

### 1. Initialize Access Point

```cpp
AccessPoint ap;
auto result = ap.init();
```

Initializes NVS, netif, event loop, and WiFi driver.

### 2. Configure AP

```cpp
APConfig config;
config.ssid = "Alloy_AP";
config.password = "alloy12345";
config.channel = 1;
config.max_connections = 4;
```

### 3. Set Callback (Optional)

```cpp
ap.set_station_callback([](bool connected, const MacAddress& mac) {
    if (connected) {
        // Station connected
    } else {
        // Station disconnected
    }
});
```

### 4. Start AP

```cpp
auto result = ap.start(config);
if (result.is_ok()) {
    // AP is running
}
```

### 5. Monitor Stations

```cpp
// Get station count
auto count = ap.station_count();

// Get station list
StationInfo stations[10];
auto result = ap.get_stations(stations, 10);
```

## Advanced Configuration

The `APConfig` struct supports additional options:

- `ssid_hidden`: Hide the SSID from scans (default: false)
- `auth_mode`: Authentication mode (Open, WPA2-PSK, etc.)

## API Reference

### AccessPoint Class

- `init()` - Initialize WiFi AP
- `start(ssid, password)` - Start AP with simple config
- `start(APConfig)` - Start AP with full config
- `stop()` - Stop the AP
- `is_running()` - Check if AP is active
- `ap_info()` - Get AP IP information
- `station_count()` - Get number of connected stations
- `get_stations()` - Get list of connected stations
- `set_station_callback()` - Set connection event callback
- `ssid()` - Get current SSID

## Troubleshooting

**AP doesn't start**:
- Check that WiFi is not already initialized in Station mode
- Verify password is at least 8 characters (or use Open mode)
- Check channel is valid (1-13)

**Can't connect from device**:
- Verify SSID and password are correct
- Check that max connections hasn't been reached
- Some devices may not support WPA2-PSK; try Open mode for testing

**Stations disconnect immediately**:
- Check ESP32 power supply (needs stable 3.3V)
- Verify WiFi channel is not congested
- Check max_connections setting

## Notes

- The default AP IP is **192.168.4.1**
- Connected devices get IPs in the range 192.168.4.2 - 192.168.4.254
- Maximum of 10 simultaneous connections (ESP32 hardware limit)
- AP mode uses less power than Station mode
- For internet access, you need to implement NAT routing (not included)

## Related Examples

- `esp32_wifi_station` - Connect to existing WiFi network
- `esp32_wifi_scanner` - Scan for WiFi networks

## License

This example is part of the Alloy framework.
