# ESP32 WiFi Station Example - ESP-IDF Project Root
#
# This example demonstrates:
# - Automatic ESP-IDF component detection
# - WiFi Station mode connection
# - HTTP GET request after connection
# - Event handling for connection/disconnection
#
# Build:
#   cd examples/esp32_wifi_station
#   idf.py build
#
# Flash:
#   idf.py -p /dev/ttyUSB0 flash monitor

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define project
project(esp32_wifi_station)
