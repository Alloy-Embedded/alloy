# ESP32 MQTT IoT Example

Comprehensive MQTT client demonstration using CoreZero's MQTT abstraction over ESP-IDF.

## Features

- ✅ WiFi Station connectivity
- ✅ MQTT client with QoS 0, 1, 2 support
- ✅ Subscribe to multiple topics with callbacks
- ✅ Publish sensor data periodically
- ✅ Wildcard topic subscriptions (`#` and `+`)
- ✅ Last Will and Testament (LWT) configuration
- ✅ Connection state management
- ✅ Error handling with `Result<T>`
- ✅ Retained messages support

## Hardware Requirements

- ESP32, ESP32-C3, ESP32-S3, or ESP32-S2
- USB cable for programming and power
- WiFi network access

## Software Requirements

- ESP-IDF v5.0 or later
- CoreZero Framework

## Configuration

Edit `main/main.cpp` and update WiFi credentials:

```cpp
constexpr const char* WIFI_SSID = "YOUR_WIFI_SSID";
constexpr const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

### MQTT Broker

The example uses the public HiveMQ broker by default (`mqtt://broker.hivemq.com`). You can change this to your own broker:

```cpp
constexpr const char* MQTT_BROKER = "mqtt://your-broker.com";
```

For secure connections (mqtts://):

```cpp
MQTT::Config mqtt_config{
    .broker_uri = "mqtts://secure-broker.com",
    .port = 8883,
    .ca_cert = ca_cert_pem_start,  // Your CA certificate
    // ... other config
};
```

## Building and Flashing

### Setup ESP-IDF

```bash
# Source ESP-IDF environment
. $HOME/esp/esp-idf/export.sh
```

### Build

```bash
./build.sh
```

Or manually:

```bash
idf.py build
```

### Flash and Monitor

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with your ESP32's serial port:
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- macOS: `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`
- Windows: `COM3`, `COM4`, etc.

## Testing

### Monitor MQTT Traffic

Use `mosquitto_sub` to monitor messages:

```bash
# Subscribe to sensor data
mosquitto_sub -h broker.hivemq.com -t 'corezero/esp32/sensor' -v

# Subscribe to all topics
mosquitto_sub -h broker.hivemq.com -t 'corezero/#' -v
```

### Send Commands

Use `mosquitto_pub` to send commands:

```bash
# Send ping command
mosquitto_pub -h broker.hivemq.com -t 'corezero/esp32/cmd' -m 'ping'

# Send status command
mosquitto_pub -h broker.hivemq.com -t 'corezero/esp32/cmd' -m 'status'

# Test broadcast
mosquitto_pub -h broker.hivemq.com -t 'corezero/broadcast/alert' -m 'System update'
```

## Expected Output

```
I (12345) MQTT_IOT: === ESP32 MQTT IoT Example ===
I (12346) MQTT_IOT: Connecting to WiFi...
I (14567) MQTT_IOT: WiFi Connected!
I (14568) MQTT_IOT:   IP Address: 192.168.1.100
I (14569) MQTT_IOT:   Gateway: 192.168.1.1
I (14570) MQTT_IOT: Initializing MQTT client...
I (15678) MQTT_IOT: MQTT Connected to broker
I (15679) MQTT_IOT: Subscribed to: corezero/esp32/cmd
I (15680) MQTT_IOT: Subscribed to: corezero/broadcast/#
I (15681) MQTT_IOT: Setup complete. Starting main loop...
I (15682) MQTT_IOT: Published sensor data: {"temperature":25.0,"humidity":60.0,"counter":0}
I (25682) MQTT_IOT: Published sensor data: {"temperature":25.5,"humidity":62.0,"counter":1}
```

## Topics

| Topic | Direction | Description | QoS |
|-------|-----------|-------------|-----|
| `corezero/esp32/status` | Publish | Device online/offline status | 1 (retained) |
| `corezero/esp32/sensor` | Publish | Sensor data (JSON) | 1 |
| `corezero/esp32/cmd` | Subscribe | Commands (ping, status) | 1 |
| `corezero/broadcast/#` | Subscribe | Broadcast messages | 0 |

## Message Formats

### Sensor Data (Published)

```json
{
  "temperature": 25.5,
  "humidity": 62.0,
  "counter": 1
}
```

### Commands (Subscribed)

- `ping` - Device logs "PING" and can respond with "PONG"
- `status` - Device sends status update

## QoS Levels

The example demonstrates different QoS levels:

- **QoS 0 (At Most Once)**: Broadcast messages - fire and forget
- **QoS 1 (At Least Once)**: Sensor data and commands - guaranteed delivery
- **QoS 2 (Exactly Once)**: Not used in this example, but supported

## Last Will and Testament (LWT)

The device configures LWT to notify subscribers if it disconnects unexpectedly:

```cpp
.lwt_topic = TOPIC_STATUS,
.lwt_message = "offline",
.lwt_qos = MQTT::QoS::AtLeastOnce,
.lwt_retain = true
```

Test this by:
1. Start monitoring: `mosquitto_sub -h broker.hivemq.com -t 'corezero/esp32/status' -v`
2. Flash and run the device
3. Observe "online" message
4. Disconnect power or press reset
5. Observe "offline" message delivered by the broker

## Troubleshooting

### WiFi Connection Failed

- Verify SSID and password in `main.cpp`
- Check WiFi signal strength
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)

### MQTT Connection Failed

- Check broker URL and port
- Verify network allows outbound connections on port 1883
- Try public broker: `mqtt://test.mosquitto.org`
- Check firewall settings

### Build Errors

- Ensure ESP-IDF v5.0+ is installed
- Run `. $HOME/esp/esp-idf/export.sh`
- Clean build: `idf.py fullclean && idf.py build`

### Messages Not Received

- Verify topic names match exactly (case-sensitive)
- Check QoS level compatibility
- Monitor broker logs if using your own broker
- Use `mosquitto_sub` to verify messages are being published

## Customization

### Add New Sensor

```cpp
void publish_custom_sensor(MQTT::Client& client) {
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer),
                      "{\"sensor_type\":\"custom\",\"value\":%d}",
                      read_sensor());

    client.publish("corezero/esp32/custom", buffer, len,
                  MQTT::QoS::AtLeastOnce);
}
```

### Handle New Command

Update `on_command_received()`:

```cpp
void on_command_received(const MQTT::Message& msg) {
    if (msg.payload() == "reboot") {
        ESP_LOGI(TAG, "Rebooting device...");
        esp_restart();
    }
}
```

### Use Secure Connection

```cpp
extern const uint8_t ca_cert_pem_start[] asm("_binary_ca_cert_pem_start");

MQTT::Config mqtt_config{
    .broker_uri = "mqtts://secure-broker.com",
    .port = 8883,
    .ca_cert = (const char*)ca_cert_pem_start,
    .skip_cert_verify = false  // Verify certificate
};
```

## API Reference

See the [MQTT API documentation](../../docs/api/mqtt.md) for complete API details.

## Performance

Typical resource usage:
- **Flash**: ~750 KB (with WiFi, MQTT, and TLS support)
- **RAM**: ~80 KB (heap usage)
- **CPU**: <5% (idle most of the time)

## Related Examples

- [esp32_wifi_station](../esp32_wifi_station/) - Basic WiFi connectivity
- [esp32_http_server](../esp32_http_server/) - HTTP server with WiFi
- [esp32_ble_scanner](../esp32_ble_scanner/) - Bluetooth Low Energy scanner

## License

This example is part of the CoreZero Framework and is released under the same license.
