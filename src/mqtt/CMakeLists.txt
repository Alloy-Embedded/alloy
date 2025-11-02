# MQTT Component
# Provides MQTT client wrapper for ESP-IDF mqtt_client

if(ESP_PLATFORM)
    idf_component_register(
        SRCS "client.cpp"
        INCLUDE_DIRS "."
        REQUIRES mqtt esp_event nvs_flash
    )
else()
    # For non-ESP32 platforms, provide stub implementation
    add_library(mqtt STATIC client.cpp)
    target_include_directories(mqtt PUBLIC .)
    target_link_libraries(mqtt PUBLIC core)
endif()
