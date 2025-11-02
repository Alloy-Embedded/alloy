# Main component for WiFi Scanner example
#
# This example demonstrates Alloy's WiFi scanning abstraction layer

# Source files for this example
set(SRCS
    "main.cpp"
)

# Alloy WiFi component path
set(ALLOY_WIFI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/wifi")

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS
        "."
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../src"
    REQUIRES
        # Base components
        esp_system
        driver
        # ESP-IDF WiFi components
        esp_wifi
        esp_netif
        nvs_flash
        esp_event
)

# Add Alloy WiFi source files to this component
target_sources(${COMPONENT_LIB} PRIVATE
    "${ALLOY_WIFI_PATH}/station.cpp"
    "${ALLOY_WIFI_PATH}/access_point.cpp"
    "${ALLOY_WIFI_PATH}/scanner.cpp"
)

# Set component compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -DESP32
)
