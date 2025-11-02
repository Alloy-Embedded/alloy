# Main component for HTTP Server example

set(SRCS "main.cpp")
set(ALLOY_WIFI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/wifi")
set(ALLOY_HTTP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/http")

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "." "${CMAKE_CURRENT_SOURCE_DIR}/../../../src"
    REQUIRES esp_system driver esp_wifi esp_netif nvs_flash esp_event esp_http_server esp_timer
)

# Add Alloy source files
target_sources(${COMPONENT_LIB} PRIVATE
    "${ALLOY_WIFI_PATH}/station.cpp"
    "${ALLOY_WIFI_PATH}/access_point.cpp"
    "${ALLOY_WIFI_PATH}/scanner.cpp"
    "${ALLOY_HTTP_PATH}/server.cpp"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -DESP32)
