# ESP32 Logger Example - Multiple Sinks Demo
#
# This example demonstrates:
# - CoreZero universal logger on ESP32
# - Multiple sinks (UART + ESP-IDF bridge)
# - Integration with ESP-IDF logging system
# - All log levels and formatting
#
# Build:
#   cd examples/esp32_logger
#   idf.py build
#
# Flash:
#   idf.py -p /dev/ttyUSB0 flash monitor

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define project
project(esp32_logger)
