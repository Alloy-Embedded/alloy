# ESP32 HAL Backend using ESP-IDF Drivers
# Provides optimized hardware drivers for ESP32 platform

if(ESP_PLATFORM)
    idf_component_register(
        INCLUDE_DIRS "."
        REQUIRES driver esp_system
    )

    # Note: This is a header-only library for template-based HAL
    # No source files needed - implementations are in headers
else()
    # For non-ESP32 platforms, this component is not available
    message(STATUS "ESP32 HAL Backend requires ESP-IDF (ESP_PLATFORM not defined)")
endif()
