# RTOS Blink example for ESP32 DevKit
#
# This example supports two build modes:
# 1. ESP-IDF Mode (Recommended): Uses ESP-IDF components and build system
# 2. Bare-Metal Mode: Pure Alloy RTOS without ESP-IDF dependencies
#
# ESP-IDF mode is automatically enabled when IDF_PATH is detected or
# when building through build-esp32.sh script.

# Check if ESP-IDF is available AND we're building as an ESP-IDF project
# Note: idf_component_register() only works when building via idf.py, not via traditional CMake
if(DEFINED IDF_BUILD_SYSTEM_VERSION)
    # Building via idf.py - use ESP-IDF mode
    set(USE_ESP_IDF TRUE)
    message(STATUS "ESP-IDF build system detected - Building with ESP-IDF integration")
    message(STATUS "  IDF_PATH: $ENV{IDF_PATH}")
else()
    # Building via traditional CMake - use bare-metal mode
    set(USE_ESP_IDF FALSE)
    message(STATUS "Building in bare-metal mode (traditional CMake)")
endif()

# ============================================================================
# ESP-IDF Mode: Use ESP-IDF component system
# ============================================================================
if(USE_ESP_IDF)
    # Register this example as an ESP-IDF component
    idf_component_register(
        SRCS
            "main.cpp"
            "../../boards/esp32_devkit/startup.cpp"
            "../../src/rtos/scheduler.cpp"
            "../../src/rtos/platform/xtensa_context.cpp"
            "../../src/rtos/platform/xtensa_systick_integration.cpp"
        INCLUDE_DIRS
            "."
            "../../src"
            "../../boards"
        REQUIRES
            # ESP-IDF components we depend on (minimal set)
            esp_system
            esp_timer
            driver
    )

    # ESP32-specific compile options for ESP-IDF
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -mtext-section-literals
        -O2
        -Wall
        -Wextra
        -DESP32
        -DUSE_ESP_IDF=1
    )

    # Add linker script from board config
    target_link_options(${COMPONENT_LIB} PRIVATE
        -mtext-section-literals
        -T ${CMAKE_SOURCE_DIR}/boards/esp32_devkit/esp32.ld
    )

    message(STATUS "ESP-IDF component registered: rtos_blink_esp32")
    message(STATUS "  Sources: main.cpp + RTOS core + board files")
    message(STATUS "  ESP-IDF Components: esp_system, esp_timer, driver")

# ============================================================================
# Bare-Metal Mode: Traditional CMake build
# ============================================================================
else()
    add_executable(rtos_blink_esp32
        main.cpp
        ../../boards/esp32_devkit/startup.cpp
        ../../src/rtos/scheduler.cpp
        ../../src/rtos/platform/xtensa_context.cpp
        ../../src/rtos/platform/xtensa_systick_integration.cpp
    )

    target_include_directories(rtos_blink_esp32 PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
    )

    # Apply board-specific flags and linker script
    alloy_target_board_settings(rtos_blink_esp32)

    # ESP32-specific compile options
    target_compile_options(rtos_blink_esp32 PRIVATE
        -mtext-section-literals
        -O2
        -Wall
        -Wextra
        -DESP32
        -DUSE_ESP_IDF=0
    )

    # ESP32-specific link options
    target_link_options(rtos_blink_esp32 PRIVATE
        -mtext-section-literals
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/rtos_blink_esp32.map
        -Wl,--print-memory-usage
        -nostartfiles
        -lc
        -lgcc
        -lstdc++
    )

    # Generate binary outputs (.bin, .hex) and print size
    alloy_add_binary_outputs(rtos_blink_esp32)

    # Create flash target (default port: /dev/ttyUSB0, baud: 115200)
    alloy_add_flash_target(rtos_blink_esp32 PORT /dev/ttyUSB0 BAUD 115200)

    message(STATUS "Bare-metal build configured")
    message(STATUS "  To enable ESP-IDF: source ~/esp/esp-idf/export.sh")
endif()
