#include "client.hpp"

#ifdef ESP_PLATFORM
    #include <cstring>

    #include "esp_log.h"
    #include "mqtt_client.h"

static const char* TAG = "MQTT::Client";

namespace MQTT {

// Internal implementation structure
struct Client::Impl {
    esp_mqtt_client_handle_t client = nullptr;
    Config config;
    State current_state = State::Disconnected;

    // Callbacks
    ConnectionCallback connection_cb;
    MessageCallback message_cb;
    PublishCallback publish_cb;
    SubscribeCallback subscribe_cb;

    // Topic-specific message callbacks
    std::map<std::string, MessageCallback> topic_callbacks;

    // Static event handler for ESP-IDF
    static void event_handler(void* handler_args, esp_event_base_t base, int32_t event_id,
                              void* event_data) {
        auto* impl = static_cast<Impl*>(handler_args);
        auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);

        switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
            case MQTT_EVENT_CONNECTED:
                impl->handle_connected();
                break;
            case MQTT_EVENT_DISCONNECTED:
                impl->handle_disconnected();
                break;
            case MQTT_EVENT_SUBSCRIBED:
                impl->handle_subscribed(event);
                break;
            case MQTT_EVENT_UNSUBSCRIBED:
                ESP_LOGI(TAG, "Unsubscribed, msg_id=%d", event->msg_id);
                break;
            case MQTT_EVENT_PUBLISHED:
                impl->handle_published(event);
                break;
            case MQTT_EVENT_DATA:
                impl->handle_data(event);
                break;
            case MQTT_EVENT_ERROR:
                impl->handle_error(event);
                break;
            default:
                break;
        }
    }

    void handle_connected() {
        ESP_LOGI(TAG, "Connected to MQTT broker");
        current_state = State::Connected;
        if (connection_cb != nullptr) {
            connection_cb(true, ErrorReason::None);
        }
    }

    void handle_disconnected() {
        ESP_LOGI(TAG, "Disconnected from MQTT broker");
        current_state = State::Disconnected;
        if (connection_cb != nullptr) {
            connection_cb(false, ErrorReason::NetworkError);
        }
    }

    void handle_subscribed(esp_mqtt_event_handle_t event) {
        ESP_LOGI(TAG, "Subscribed, msg_id=%d", event->msg_id);
        if (subscribe_cb != nullptr) {
            subscribe_cb("", true);  // Topic not available in event
        }
    }

    void handle_published(esp_mqtt_event_handle_t event) {
        ESP_LOGD(TAG, "Published, msg_id=%d", event->msg_id);
        if (publish_cb != nullptr) {
            publish_cb(event->msg_id, true);
        }
    }

    void handle_data(esp_mqtt_event_handle_t event) {
        // Create message from event data
        Message msg{.topic = std::string_view(event->topic, event->topic_len),
                    .data = reinterpret_cast<const uint8_t*>(event->data),
                    .length = static_cast<size_t>(event->data_len),
                    .qos = static_cast<QoS>(event->qos),
                    .retained = event->retain};

        ESP_LOGD(TAG, "Received message on topic: %.*s, length: %d", event->topic_len, event->topic,
                 event->data_len);

        // Check for topic-specific callback
        bool handled = false;
        for (const auto& [pattern, callback] : topic_callbacks) {
            if (topic_matches(msg.topic, pattern)) {
                callback(msg);
                handled = true;
                break;
            }
        }

        // Use default message callback if no specific handler
        if (!handled && message_cb != nullptr) {
            message_cb(msg);
        }
    }

    void handle_error(esp_mqtt_event_handle_t event) {
        ESP_LOGE(TAG, "MQTT error event");
        current_state = State::Error;
        if (connection_cb != nullptr) {
            connection_cb(false, ErrorReason::Unknown);
        }
    }

    // Simple topic pattern matching (supports + and # wildcards)
    static bool topic_matches(std::string_view topic, const std::string& pattern) {
        // Simple implementation - exact match for now
        // TODO: Implement full MQTT wildcard matching
        return topic == pattern;
    }
};

Client::Client(const Config& config) : impl_(std::make_unique<Impl>()) {
    impl_->config = config;

    // Configure ESP-IDF MQTT client
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = config.broker_uri;

    if (config.port > 0) {
        mqtt_cfg.broker.address.port = config.port;
    }

    if (config.client_id != nullptr) {
        mqtt_cfg.credentials.client_id = config.client_id;
    }

    if (config.username != nullptr) {
        mqtt_cfg.credentials.username = config.username;
    }

    if (config.password != nullptr) {
        mqtt_cfg.credentials.authentication.password = config.password;
    }

    mqtt_cfg.session.keepalive = config.keepalive;
    mqtt_cfg.session.disable_clean_session = !config.clean_session;

    // TLS/SSL configuration
    if (config.ca_cert != nullptr) {
        mqtt_cfg.broker.verification.certificate = config.ca_cert;
    }

    if (config.client_cert != nullptr) {
        mqtt_cfg.credentials.authentication.certificate = config.client_cert;
    }

    if (config.client_key != nullptr) {
        mqtt_cfg.credentials.authentication.key = config.client_key;
    }

    if (config.skip_cert_verify) {
        mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
    }

    // Last Will and Testament
    if (config.lwt_topic != nullptr) {
        mqtt_cfg.session.last_will.topic = config.lwt_topic;
        mqtt_cfg.session.last_will.msg = config.lwt_message;
        mqtt_cfg.session.last_will.msg_len = config.lwt_message != nullptr ? strlen(config.lwt_message) : 0;
        mqtt_cfg.session.last_will.qos = static_cast<int>(config.lwt_qos);
        mqtt_cfg.session.last_will.retain = config.lwt_retain;
    }

    // Create client
    impl_->client = esp_mqtt_client_init(&mqtt_cfg);
    if (impl_->client == nullptr) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    // Register event handler
    esp_mqtt_client_register_event(impl_->client, MQTT_EVENT_ANY, Impl::event_handler, impl_.get());
}

Client::~Client() {
    if (impl_ && impl_->client != nullptr) {
        disconnect();
        esp_mqtt_client_destroy(impl_->client);
    }
}

Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

Result<void, ErrorCode> Client::connect() {
    if (!impl_ || impl_->client == nullptr) {
        return Err(ErrorCode::Generic);
    }

    impl_->current_state = State::Connecting;
    esp_err_t err = esp_mqtt_client_start(impl_->client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        impl_->current_state = State::Error;
        return Err(ErrorCode::Communication);
    }

    return Ok();
}

void Client::disconnect() {
    if (impl_ && impl_->client != nullptr) {
        esp_mqtt_client_stop(impl_->client);
        impl_->current_state = State::Disconnected;
    }
}

bool Client::is_connected() const {
    return impl_ && impl_->current_state == State::Connected;
}

State Client::state() const {
    return impl_ ? impl_->current_state : State::Error;
}

Result<int, ErrorCode> Client::publish(const char* topic, const void* data, size_t length, QoS qos,
                                       bool retain) {
    if (!is_connected()) {
        return Err(ErrorCode::Communication);
    }

    int msg_id = esp_mqtt_client_publish(impl_->client, topic, static_cast<const char*>(data),
                                         length, static_cast<int>(qos), retain ? 1 : 0);

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish message");
        return Err(ErrorCode::Communication);
    }

    ESP_LOGD(TAG, "Published to %s, msg_id=%d", topic, msg_id);
    return Ok(msg_id);
}

Result<int, ErrorCode> Client::publish(const char* topic, const char* message, QoS qos,
                                       bool retain) {
    return publish(topic, message, strlen(message), qos, retain);
}

Result<void, ErrorCode> Client::subscribe(const char* topic, QoS qos, MessageCallback callback) {
    if (!is_connected()) {
        return Err(ErrorCode::Communication);
    }

    // Store topic-specific callback
    if (callback != nullptr) {
        impl_->topic_callbacks[topic] = std::move(callback);
    }

    int msg_id = esp_mqtt_client_subscribe(impl_->client, topic, static_cast<int>(qos));

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to subscribe to %s", topic);
        impl_->topic_callbacks.erase(topic);
        return Err(ErrorCode::Communication);
    }

    ESP_LOGI(TAG, "Subscribing to %s, msg_id=%d", topic, msg_id);
    return Ok();
}

Result<void, ErrorCode> Client::subscribe(const char* topic, QoS qos) {
    return subscribe(topic, qos, nullptr);
}

Result<void, ErrorCode> Client::unsubscribe(const char* topic) {
    if (!is_connected()) {
        return Err(ErrorCode::Communication);
    }

    int msg_id = esp_mqtt_client_unsubscribe(impl_->client, topic);

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to unsubscribe from %s", topic);
        return Err(ErrorCode::Communication);
    }

    // Remove topic-specific callback
    impl_->topic_callbacks.erase(topic);

    ESP_LOGI(TAG, "Unsubscribing from %s, msg_id=%d", topic, msg_id);
    return Ok();
}

void Client::set_connection_callback(ConnectionCallback callback) {
    if (impl_) {
        impl_->connection_cb = std::move(callback);
    }
}

void Client::set_message_callback(MessageCallback callback) {
    if (impl_) {
        impl_->message_cb = std::move(callback);
    }
}

void Client::set_publish_callback(PublishCallback callback) {
    if (impl_) {
        impl_->publish_cb = std::move(callback);
    }
}

void Client::set_subscribe_callback(SubscribeCallback callback) {
    if (impl_) {
        impl_->subscribe_cb = std::move(callback);
    }
}

}  // namespace MQTT

#else  // !ESP_PLATFORM

// Stub implementation for non-ESP32 platforms
namespace MQTT {

struct Client::Impl {};

Client::Client(const Config&) : impl_(std::make_unique<Impl>()) {}
Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

Result<void, ErrorCode> Client::connect() {
    return Err(ErrorCode::Generic);
}

void Client::disconnect() {}
bool Client::is_connected() const {
    return false;
}
State Client::state() const {
    return State::Disconnected;
}

Result<int, ErrorCode> Client::publish(const char*, const void*, size_t, QoS, bool) {
    return Err(ErrorCode::Generic);
}

Result<int, ErrorCode> Client::publish(const char*, const char*, QoS, bool) {
    return Err(ErrorCode::Generic);
}

Result<void, ErrorCode> Client::subscribe(const char*, QoS, MessageCallback) {
    return Err(ErrorCode::Generic);
}

Result<void, ErrorCode> Client::subscribe(const char*, QoS) {
    return Err(ErrorCode::Generic);
}

Result<void, ErrorCode> Client::unsubscribe(const char*) {
    return Err(ErrorCode::Generic);
}

void Client::set_connection_callback(ConnectionCallback) {}
void Client::set_message_callback(MessageCallback) {}
void Client::set_publish_callback(PublishCallback) {}
void Client::set_subscribe_callback(SubscribeCallback) {}

}  // namespace MQTT

#endif  // ESP_PLATFORM
