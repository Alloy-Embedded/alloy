# ESP32 WiFi Scanner Example

This example demonstrates how to use Alloy's WiFi Scanner abstraction to scan for nearby WiFi networks and display detailed information about them.

## Features

- **Network Discovery**: Scan for all WiFi networks in range
- **Detailed Information**: Display SSID, RSSI, channel, BSSID, and security type
- **Signal Quality**: Visual signal strength indicators and quality ratings
- **Blocking & Async Modes**: Support for both synchronous and asynchronous scanning
- **Targeted Scanning**: Search for specific networks by SSID
- **Statistics**: Network security type breakdown
- **Continuous Scanning**: Automatic re-scanning at intervals
- **Result<T> Error Handling**: Type-safe error handling without exceptions

## Hardware Required

- ESP32 development board (any variant with WiFi support)
- USB cable for programming and power

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
I (xxx) wifi_scanner: ========================================
I (xxx) wifi_scanner:   Alloy WiFi Scanner Example
I (xxx) wifi_scanner:   Using Alloy WiFi API
I (xxx) wifi_scanner: ========================================
I (xxx) wifi_scanner:
I (xxx) wifi_scanner: Initializing WiFi scanner...
I (xxx) wifi_scanner: ✓ WiFi scanner initialized
I (xxx) wifi_scanner:
I (xxx) wifi_scanner: Starting WiFi scan (blocking mode)...
I (xxx) wifi_scanner: Scan completed! Found 12 networks.
I (xxx) wifi_scanner:
I (xxx) wifi_scanner: ╔═══════════════════════════════════════════════════════════════════════════╗
I (xxx) wifi_scanner: ║                         WiFi Networks Found: 12                           ║
I (xxx) wifi_scanner: ╠═══════════════════════════════════════════════════════════════════════════╣
I (xxx) wifi_scanner: ║                                                                           ║
I (xxx) wifi_scanner: ║ [ 1] MyHomeNetwork                                                        ║
I (xxx) wifi_scanner: ║      Signal: ▂▄▆█ -45 dBm  Excellent                                     ║
I (xxx) wifi_scanner: ║      Channel: 6     Security: WPA2-PSK                                    ║
I (xxx) wifi_scanner: ║      BSSID: AA:BB:CC:DD:EE:FF                                             ║
I (xxx) wifi_scanner: ║                                                                           ║
I (xxx) wifi_scanner: ║ [ 2] NeighborWiFi                                                         ║
I (xxx) wifi_scanner: ║      Signal: ▂▄▆_ -58 dBm  Good                                          ║
I (xxx) wifi_scanner: ║      Channel: 11    Security: WPA2/WPA3-PSK                               ║
I (xxx) wifi_scanner: ║      BSSID: 11:22:33:44:55:66                                             ║
...
```

## How It Works

### 1. Initialize Scanner

```cpp
Scanner scanner;
auto result = scanner.init();
```

Initializes WiFi and prepares for scanning.

### 2. Perform Blocking Scan

```cpp
// Simple scan with default timeout
auto result = scanner.scan(5000);  // 5 second timeout

if (result.is_ok()) {
    uint8_t count = result.value();
    // Get results...
}
```

### 3. Get Scan Results

```cpp
uint8_t count = scanner.result_count();
AccessPointInfo networks[count];

auto result = scanner.get_results(networks, count);
if (result.is_ok()) {
    for (uint8_t i = 0; i < result.value(); i++) {
        printf("SSID: %s, RSSI: %d dBm\n",
            networks[i].ssid, networks[i].rssi);
    }
}
```

### 4. Async Scan with Callback

```cpp
// Set callback
scanner.set_scan_callback([](bool success, uint8_t count) {
    if (success) {
        // Scan completed, get results
    }
});

// Start async scan
ScanConfig config;
config.timeout_ms = 5000;
scanner.scan_async(config);
```

### 5. Targeted Scan

```cpp
// Search for specific network
ScanConfig config;
config.ssid = "TargetNetwork";
config.timeout_ms = 3000;

auto result = scanner.scan(config);
```

## Advanced Features

### Scan Configuration

The `ScanConfig` struct supports:

- `ssid` - Target specific SSID (nullptr for all)
- `bssid` - Target specific BSSID (nullptr for all)
- `channel` - Target specific channel (0 for all channels 1-14)
- `show_hidden` - Include hidden networks (default: false)
- `timeout_ms` - Scan timeout in milliseconds (default: 5000)

### Network Information

Each `AccessPointInfo` contains:

- `ssid` - Network name (max 32 chars + null)
- `bssid` - MAC address of AP
- `channel` - WiFi channel (1-14)
- `rssi` - Signal strength in dBm (-100 to 0)
- `auth_mode` - Security type (Open, WEP, WPA2, WPA3, etc.)

### Signal Strength Interpretation

| RSSI (dBm) | Quality   | Description                    |
|------------|-----------|--------------------------------|
| -50 or better | Excellent | Maximum performance           |
| -51 to -60 | Good      | Very reliable connection      |
| -61 to -70 | Fair      | Adequate, may have issues     |
| -71 to -80 | Weak      | Unreliable, frequent drops    |
| -81 or worse | Very Weak | Barely usable or unusable    |

## Use Cases

### 1. Network Discovery App

Scan and display all available networks for user selection.

### 2. Connection Helper

Find the best channel and network for optimal connectivity.

### 3. Site Survey

Map WiFi coverage in a location.

### 4. Security Audit

Identify open networks or insecure authentication modes.

### 5. Auto-Connect

Search for known networks and connect automatically.

## API Reference

### Scanner Class

- `init()` - Initialize scanner
- `scan(timeout_ms)` - Blocking scan with timeout
- `scan(ScanConfig)` - Blocking scan with configuration
- `scan_async(ScanConfig)` - Non-blocking async scan
- `get_results(array, max)` - Retrieve scan results
- `result_count()` - Get number of results from last scan
- `is_scanning()` - Check if scan is in progress
- `set_scan_callback()` - Set async scan completion callback

### ScanConfig Structure

```cpp
struct ScanConfig {
    const char* ssid;       // Target SSID (nullptr = all)
    const uint8_t* bssid;   // Target BSSID (nullptr = all)
    uint8_t channel;        // Target channel (0 = all)
    bool show_hidden;       // Include hidden networks
    uint32_t timeout_ms;    // Scan timeout
};
```

### AccessPointInfo Structure

```cpp
struct AccessPointInfo {
    char ssid[33];          // SSID (max 32 + null)
    MacAddress bssid;       // MAC address
    uint8_t channel;        // Channel (1-14)
    int8_t rssi;            // Signal strength (dBm)
    AuthMode auth_mode;     // Security type
};
```

## Customization

### Change Scan Interval

In `main.cpp`, modify the delay:

```cpp
vTaskDelay(pdMS_TO_TICKS(10000));  // Change 10000 to desired ms
```

### Filter Results

Add custom filtering in `print_scan_results()`:

```cpp
// Only show networks with good signal
if (ap.rssi >= -60) {
    // Print network
}
```

### Sort by Different Criteria

Modify `compare_by_rssi()` to sort by:
- Channel number
- Security type
- SSID alphabetically

## Troubleshooting

**No networks found**:
- Ensure WiFi is enabled on nearby devices
- Check antenna connection (if external)
- Try scanning from different locations
- Increase scan timeout

**Scan fails**:
- Verify WiFi initialization succeeded
- Check that WiFi isn't already in use by Station/AP
- Ensure NVS is properly initialized

**Missing networks**:
- Some 5GHz networks won't appear (ESP32 is 2.4GHz only)
- Hidden networks require `show_hidden = true`
- Weak networks may not be detected

**Slow scanning**:
- Scanning all channels takes time (typical: 2-4 seconds)
- Target specific channels for faster results
- Use async mode for non-blocking operation

## Performance Notes

- **Scan Time**: Full scan typically takes 2-4 seconds
- **Channel Scan**: Scanning single channel is much faster (~100ms)
- **Memory Usage**: Each AccessPointInfo is ~40 bytes
- **Max Results**: ESP-IDF typically returns up to 20-30 networks
- **Power Consumption**: Scanning uses more power than idle

## Related Examples

- `esp32_wifi_station` - Connect to WiFi network
- `esp32_wifi_ap` - Create WiFi access point

## License

This example is part of the Alloy framework.
