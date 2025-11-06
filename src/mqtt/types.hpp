#pragma once

#include <stdint.h>
#include <functional>
#include <string_view>

namespace MQTT {

/**
 * @brief MQTT Quality of Service levels
 */
enum class QoS : uint8_t {
    AtMostOnce = 0,   ///< QoS 0: At most once delivery
    AtLeastOnce = 1,  ///< QoS 1: At least once delivery
    ExactlyOnce = 2   ///< QoS 2: Exactly once delivery
};

/**
 * @brief MQTT client connection state
 */
enum class State {
    Disconnected,  ///< Not connected to broker
    Connecting,    ///< Connection in progress
    Connected,     ///< Connected to broker
    Error          ///< Error state
};

/**
 * @brief MQTT connection error reasons
 */
enum class ErrorReason {
    None,
    NetworkError,
    ProtocolError,
    AuthenticationFailed,
    ConnectionRefused,
    Timeout,
    Unknown
};

/**
 * @brief MQTT message data
 */
struct Message {
    std::string_view topic;    ///< Message topic
    const uint8_t* data;       ///< Message payload data
    size_t length;             ///< Payload length in bytes
    QoS qos;                   ///< Quality of Service level
    bool retained;             ///< Retained message flag

    /**
     * @brief Get message payload as string view
     */
    std::string_view payload() const {
        return std::string_view(reinterpret_cast<const char*>(data), length);
    }
};

/**
 * @brief MQTT client configuration
 */
struct Config {
    const char* broker_uri;           ///< Broker URI (mqtt://host:port or mqtts://host:port)
    const char* client_id = nullptr;  ///< Client ID (nullptr = auto-generated)
    const char* username = nullptr;   ///< Username for authentication
    const char* password = nullptr;   ///< Password for authentication
    uint16_t port = 0;                ///< Port (0 = use default: 1883 for mqtt, 8883 for mqtts)
    uint32_t keepalive = 120;         ///< Keep-alive interval in seconds
    bool clean_session = true;        ///< Clean session flag

    // TLS/SSL Configuration
    const char* ca_cert = nullptr;        ///< CA certificate (PEM format)
    const char* client_cert = nullptr;    ///< Client certificate (PEM format)
    const char* client_key = nullptr;     ///< Client private key (PEM format)
    bool skip_cert_verify = false;        ///< Skip certificate verification (insecure)

    // Last Will and Testament
    const char* lwt_topic = nullptr;      ///< Last will topic
    const char* lwt_message = nullptr;    ///< Last will message
    QoS lwt_qos = QoS::AtMostOnce;       ///< Last will QoS
    bool lwt_retain = false;              ///< Last will retain flag
};

/**
 * @brief Callback type for connection events
 *
 * @param connected True if connected, false if disconnected
 * @param reason Error reason if disconnected
 */
using ConnectionCallback = std::function<void(bool connected, ErrorReason reason)>;

/**
 * @brief Callback type for incoming messages
 *
 * @param message The received MQTT message
 */
using MessageCallback = std::function<void(const Message& message)>;

/**
 * @brief Callback type for publish events
 *
 * @param message_id Message ID of the published message
 * @param success True if publish succeeded
 */
using PublishCallback = std::function<void(int message_id, bool success)>;

/**
 * @brief Callback type for subscription events
 *
 * @param topic The subscribed topic
 * @param success True if subscription succeeded
 */
using SubscribeCallback = std::function<void(const char* topic, bool success)>;

} // namespace MQTT
