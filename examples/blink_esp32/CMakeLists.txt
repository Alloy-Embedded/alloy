# Blink example for ESP32 DevKit
#
# This example supports two build modes:
# 1. ESP-IDF Mode: Uses ESP-IDF components and build system
# 2. Bare-Metal Mode: Pure Alloy framework without ESP-IDF

# Check if ESP-IDF is available AND we're building as an ESP-IDF project
# Note: idf_component_register() only works when building via idf.py, not via traditional CMake
if(DEFINED IDF_BUILD_SYSTEM_VERSION)
    # Building via idf.py - use ESP-IDF mode
    set(USE_ESP_IDF TRUE)
    message(STATUS "ESP-IDF build system detected - Building blink_esp32 with ESP-IDF")
else()
    # Building via traditional CMake - use bare-metal mode
    set(USE_ESP_IDF FALSE)
    message(STATUS "Building blink_esp32 in bare-metal mode (traditional CMake)")
endif()

# ============================================================================
# ESP-IDF Mode
# ============================================================================
if(USE_ESP_IDF)
    idf_component_register(
        SRCS
            "main.cpp"
            "../../boards/esp32_devkit/startup.cpp"
        INCLUDE_DIRS
            "."
            "../../src"
            "../../boards"
        REQUIRES
            esp_system
            driver
    )

    target_compile_options(${COMPONENT_LIB} PRIVATE
        -mtext-section-literals
        -O2
        -Wall
        -Wextra
        -DUSE_ESP_IDF=1
    )

    target_link_options(${COMPONENT_LIB} PRIVATE
        -mtext-section-literals
        -T ${CMAKE_SOURCE_DIR}/boards/esp32_devkit/esp32.ld
    )

# ============================================================================
# Bare-Metal Mode
# ============================================================================
else()
    add_executable(blink_esp32
        main.cpp
        ../../boards/esp32_devkit/startup.cpp
    )

    target_include_directories(blink_esp32 PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
    )

    # Apply board-specific flags and linker script
    alloy_target_board_settings(blink_esp32)

    # ESP32-specific compile options
    target_compile_options(blink_esp32 PRIVATE
        -mtext-section-literals
        -O2
        -Wall
        -Wextra
        -DUSE_ESP_IDF=0
    )

    # ESP32-specific link options
    target_link_options(blink_esp32 PRIVATE
        -mtext-section-literals
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_esp32.map
        -Wl,--print-memory-usage
        -nostartfiles
        -lc
        -lgcc
        -lstdc++
    )

    # Generate binary outputs (.bin, .hex) and print size
    alloy_add_binary_outputs(blink_esp32)

    # Create flash target (default port: /dev/ttyUSB0, baud: 115200)
    alloy_add_flash_target(blink_esp32 PORT /dev/ttyUSB0 BAUD 115200)
endif()
