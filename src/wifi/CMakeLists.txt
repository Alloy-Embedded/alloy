# Alloy WiFi Component
#
# C++ abstraction layer for ESP-IDF WiFi functionality

set(SRCS
    "station.cpp"
    "access_point.cpp"
    "scanner.cpp"
)

set(INCLUDE_DIRS
    "."
    "../core"
)

# Register component
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
)

# Set C++20 compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=c++20
    -Wall
    -Wextra
    -DESP_PLATFORM
)
