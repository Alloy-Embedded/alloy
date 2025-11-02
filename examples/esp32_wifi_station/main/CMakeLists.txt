# Main component for WiFi Station example
#
# This CMakeLists.txt uses CoreZero's automatic component detection.
# ESP-IDF components are automatically detected from #include statements.

# Source files for this example
set(SRCS
    "main.cpp"
)

# Use CoreZero's automatic component detection helper
# It will scan SRCS for ESP-IDF includes and automatically add required components
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "."
    REQUIRES
        # Base components
        esp_system
        driver
        # WiFi components will be auto-detected from includes in main.cpp:
        # #include <esp_wifi.h> -> esp_wifi, esp_netif, nvs_flash, wpa_supplicant
        # #include <esp_http_client.h> -> esp_http_client, esp-tls
        # But we can also list them explicitly if we want:
        esp_wifi
        esp_netif
        nvs_flash
        esp_http_client
        esp_event
)

# Set component compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -DESP32
)
