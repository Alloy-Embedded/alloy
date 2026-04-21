# ESP-IDF Auto-Integration for Alloy Framework
#
# This module automatically sets up ESP-IDF when building for ESP32 boards.
# Users don't need to install or configure ESP-IDF manually.
#
# Usage (automatic):
#   cmake -B build -DALLOY_BOARD=esp32_devkit
#   cmake --build build
#
# The system will:
# 1. Check if ESP-IDF is available as submodule
# 2. If not, try to find system ESP-IDF installation
# 3. If not found, provide clear instructions
# 4. Configure ESP-IDF build system transparently

# Guard against multiple inclusion
if(ALLOY_ESP32_INTEGRATION_INCLUDED)
    return()
endif()
set(ALLOY_ESP32_INTEGRATION_INCLUDED TRUE)

message(STATUS "========================================")
message(STATUS "ESP32 Build System Integration")
message(STATUS "========================================")

# Path to ESP-IDF (priority order)
set(ESP_IDF_PATHS
    "${CMAKE_SOURCE_DIR}/external/esp-idf"           # Submodule (preferred)
    "$ENV{IDF_PATH}"                                  # Environment variable
    "$ENV{HOME}/esp/esp-idf"                         # Standard install location
    "/opt/esp-idf"                                    # System install
)

# Find ESP-IDF
set(ESP_IDF_FOUND FALSE)
foreach(path ${ESP_IDF_PATHS})
    if(EXISTS "${path}/tools/cmake/idf.cmake")
        set(IDF_PATH "${path}")
        set(ESP_IDF_FOUND TRUE)
        message(STATUS "Found ESP-IDF at: ${IDF_PATH}")
        break()
    endif()
endforeach()

if(NOT ESP_IDF_FOUND)
    message(STATUS "ESP-IDF not found. Checking for submodule...")

    # Check if external/esp-idf exists but is empty (submodule not initialized)
    if(EXISTS "${CMAKE_SOURCE_DIR}/external/esp-idf" AND
       NOT EXISTS "${CMAKE_SOURCE_DIR}/external/esp-idf/tools/cmake/idf.cmake")
        message(STATUS "ESP-IDF submodule found but not initialized.")
        message(STATUS "")
        message(STATUS "To initialize ESP-IDF submodule:")
        message(STATUS "  git submodule update --init --recursive external/esp-idf")
        message(STATUS "")
        message(STATUS "Or clone manually:")
        message(STATUS "  git clone --recursive https://github.com/espressif/esp-idf.git external/esp-idf")
        message(STATUS "  cd external/esp-idf && ./install.sh esp32")
        message(FATAL_ERROR "ESP-IDF submodule not initialized")
    endif()

    # ESP-IDF not found anywhere
    message(STATUS "")
    message(STATUS "ESP-IDF not found. Please install:")
    message(STATUS "")
    message(STATUS "Option 1 - As submodule (recommended):")
    message(STATUS "  cd ${CMAKE_SOURCE_DIR}")
    message(STATUS "  git submodule add https://github.com/espressif/esp-idf.git external/esp-idf")
    message(STATUS "  git submodule update --init --recursive")
    message(STATUS "  cd external/esp-idf && ./install.sh esp32")
    message(STATUS "")
    message(STATUS "Option 2 - System installation:")
    message(STATUS "  mkdir -p ~/esp")
    message(STATUS "  cd ~/esp")
    message(STATUS "  git clone --recursive https://github.com/espressif/esp-idf.git")
    message(STATUS "  cd esp-idf && ./install.sh esp32")
    message(STATUS "  export IDF_PATH=~/esp/esp-idf")
    message(STATUS "")
    message(FATAL_ERROR "ESP-IDF required for ESP32 builds")
endif()

# Set ESP-IDF environment
set(ENV{IDF_PATH} "${IDF_PATH}")

# ESP-IDF component configuration for Alloy
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/boards"
)

# Disable ESP-IDF components not used by the bare-metal HAL runtime.
set(EXCLUDE_COMPONENTS
    "bt"                # Bluetooth
    "lwip"              # TCP/IP stack
    "mbedtls"           # TLS
    "esp_http_client"   # HTTP client
    "esp_http_server"   # HTTP server
    "mqtt"              # MQTT client
    "fatfs"             # FAT filesystem
    "wear_levelling"    # Flash wear levelling
    "spiffs"            # SPIFFS filesystem
)

# ESP32 chip target
set(IDF_TARGET "esp32" CACHE STRING "ESP-IDF target chip")

# Include ESP-IDF build system
include("${IDF_PATH}/tools/cmake/idf.cmake")

# Validate ESP-IDF version
if(DEFINED IDF_VERSION_MAJOR AND DEFINED IDF_VERSION_MINOR)
    if(IDF_VERSION_MAJOR LESS 5)
        message(WARNING "ESP-IDF version ${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR} detected.")
        message(WARNING "Alloy requires ESP-IDF >= 5.0 for full feature support.")
        message(WARNING "Some features may not work correctly.")
    endif()
endif()

message(STATUS "ESP-IDF Version: ${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")
message(STATUS "Target Chip: ${IDF_TARGET}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")

# Function: Register Alloy application as ESP-IDF component with auto-detection
# Usage: alloy_esp32_component(
#            SRCS source1.cpp source2.cpp ...
#            [INCLUDE_DIRS dir1 dir2 ...]
#            [REQUIRES comp1 comp2 ...]  # Additional manual components
#            [NO_AUTO_DETECT]            # Disable automatic component detection
#        )
function(alloy_esp32_component)
    cmake_parse_arguments(
        ARG
        "NO_AUTO_DETECT"                    # Options
        ""                                  # One-value keywords
        "SRCS;INCLUDE_DIRS;REQUIRES"        # Multi-value keywords
        ${ARGN}
    )

    # Default include directories
    set(DEFAULT_INCLUDES
        "."
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/boards"
    )

    # Merge with user-provided include directories
    if(ARG_INCLUDE_DIRS)
        list(APPEND DEFAULT_INCLUDES ${ARG_INCLUDE_DIRS})
    endif()

    # Minimum required components for Alloy
    set(BASE_COMPONENTS esp_system driver)

    # Automatic component detection (unless disabled)
    if(NOT ARG_NO_AUTO_DETECT)
        alloy_detect_esp_components(AUTO_COMPONENTS ${ARG_SRCS})
        if(AUTO_COMPONENTS)
            message(STATUS "Auto-detected ESP-IDF components: ${AUTO_COMPONENTS}")
            list(APPEND BASE_COMPONENTS ${AUTO_COMPONENTS})
        endif()
    endif()

    # Add user-specified components
    if(ARG_REQUIRES)
        list(APPEND BASE_COMPONENTS ${ARG_REQUIRES})
    endif()

    # Remove duplicates
    list(REMOVE_DUPLICATES BASE_COMPONENTS)

    # Register as ESP-IDF component
    idf_component_register(
        SRCS ${ARG_SRCS}
        INCLUDE_DIRS ${DEFAULT_INCLUDES}
        REQUIRES ${BASE_COMPONENTS}
    )

    # Set Alloy-specific compile options
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -DUSE_ESP_IDF=1
        -Wall
        -Wextra
    )

    message(STATUS "Registered Alloy component with ESP-IDF components: ${BASE_COMPONENTS}")
endfunction()
