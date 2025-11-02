# ESP32 MQTT IoT Example
# Demonstrates MQTT client with WiFi connectivity

cmake_minimum_required(VERSION 3.16)

# Set Alloy root
get_filename_component(ALLOY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Use ESP-IDF build system
set(EXTRA_COMPONENT_DIRS
    "${ALLOY_ROOT}/src/wifi"
    "${ALLOY_ROOT}/src/mqtt"
    "${ALLOY_ROOT}/src/core"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_mqtt_iot)
