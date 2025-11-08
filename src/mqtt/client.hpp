#pragma once

#include <map>
#include <memory>
#include <string>

#include "../core/error.hpp"
#include "../core/result.hpp"
#include "types.hpp"

namespace MQTT {

/**
 * @brief MQTT Client with RAII pattern
 *
 * Provides a modern C++ interface to MQTT functionality, wrapping ESP-IDF's
 * mqtt_client component. Handles connection lifecycle, pub/sub operations,
 * and callbacks.
 *
 * Example:
 * @code
 * MQTT::Config config{
 *     .broker_uri = "mqtt://broker.hivemq.com",
 *     .client_id = "esp32_client"
 * };
 *
 * MQTT::Client client(config);
 *
 * // Set up callbacks
 * client.set_connection_callback([](bool connected, ErrorReason reason) {
 *     if (connected) {
 *         Serial.println("Connected to MQTT broker");
 *     }
 * });
 *
 * // Connect
 * auto result = client.connect();
 * if (result.is_ok()) {
 *     // Subscribe to topic
 *     client.subscribe("sensors/+/temperature", MQTT::QoS::AtLeastOnce,
 *         [](const MQTT::Message& msg) {
 *             Serial.printf("Received: %.*s\n", msg.length, msg.data);
 *         });
 *
 *     // Publish message
 *     client.publish("sensors/esp32/status", "online", MQTT::QoS::AtLeastOnce);
 * }
 * @endcode
 */
class Client {
   public:
    /**
     * @brief Construct MQTT client with configuration
     *
     * @param config MQTT client configuration
     */
    explicit Client(const Config& config);

    /**
     * @brief Destructor - disconnects and cleans up resources
     */
    ~Client();

    // Non-copyable, movable
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    /**
     * @brief Connect to MQTT broker
     *
     * Initiates connection to the broker specified in configuration.
     * Connection result is reported via connection callback.
     *
     * @return Result<void> - Ok if connection initiated successfully
     */
    Result<void, ErrorCode> connect();

    /**
     * @brief Disconnect from MQTT broker
     *
     * Cleanly disconnects from the broker.
     */
    void disconnect();

    /**
     * @brief Check if connected to broker
     *
     * @return True if connected, false otherwise
     */
    [[nodiscard]] bool is_connected() const;

    /**
     * @brief Get current connection state
     *
     * @return Current State
     */
    [[nodiscard]] State state() const;

    /**
     * @brief Publish message to topic
     *
     * @param topic Topic to publish to
     * @param data Message payload data
     * @param length Payload length in bytes
     * @param qos Quality of Service level
     * @param retain Retain message flag
     * @return Result<int> - Message ID if successful
     */
    Result<int, ErrorCode> publish(const char* topic, const void* data, size_t length,
                                   QoS qos = QoS::AtMostOnce, bool retain = false);

    /**
     * @brief Publish string message to topic
     *
     * @param topic Topic to publish to
     * @param message Message string
     * @param qos Quality of Service level
     * @param retain Retain message flag
     * @return Result<int> - Message ID if successful
     */
    Result<int, ErrorCode> publish(const char* topic, const char* message,
                                   QoS qos = QoS::AtMostOnce, bool retain = false);

    /**
     * @brief Subscribe to topic with callback
     *
     * @param topic Topic pattern to subscribe to (supports wildcards: + and #)
     * @param qos Quality of Service level
     * @param callback Callback for incoming messages on this topic
     * @return Result<void> - Ok if subscription initiated successfully
     */
    Result<void, ErrorCode> subscribe(const char* topic, QoS qos, MessageCallback callback);

    /**
     * @brief Subscribe to topic (uses default message callback)
     *
     * @param topic Topic pattern to subscribe to
     * @param qos Quality of Service level
     * @return Result<void> - Ok if subscription initiated successfully
     */
    Result<void, ErrorCode> subscribe(const char* topic, QoS qos = QoS::AtMostOnce);

    /**
     * @brief Unsubscribe from topic
     *
     * @param topic Topic to unsubscribe from
     * @return Result<void> - Ok if unsubscribe initiated successfully
     */
    Result<void, ErrorCode> unsubscribe(const char* topic);

    /**
     * @brief Set connection state callback
     *
     * Called when connection state changes (connected/disconnected).
     *
     * @param callback Connection callback function
     */
    void set_connection_callback(ConnectionCallback callback);

    /**
     * @brief Set default message callback
     *
     * Called for messages on subscribed topics that don't have
     * a specific callback set.
     *
     * @param callback Message callback function
     */
    void set_message_callback(MessageCallback callback);

    /**
     * @brief Set publish callback
     *
     * Called when a publish operation completes.
     *
     * @param callback Publish callback function
     */
    void set_publish_callback(PublishCallback callback);

    /**
     * @brief Set subscribe callback
     *
     * Called when a subscribe operation completes.
     *
     * @param callback Subscribe callback function
     */
    void set_subscribe_callback(SubscribeCallback callback);

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace MQTT
