# Blink example for ESP32 DevKit
#
# This example supports two build modes:
# 1. ESP-IDF Mode: Uses ESP-IDF components and build system
# 2. Bare-Metal Mode: Pure Alloy framework without ESP-IDF

# Check if ESP-IDF integration is enabled
# ESP-IDF is available when IDF_PATH is set and idf.cmake has been included
if(COMMAND idf_build_component)
    # ESP-IDF build system is available - use ESP-IDF mode
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
    # Use CoreZero's ESP-IDF component helper with automatic component detection
    alloy_esp32_component(
        SRCS
            "main.cpp"
            "../../boards/esp32_devkit/startup.cpp"
        INCLUDE_DIRS
            "."
            "../../src"
            "../../boards"
        # REQUIRES is optional - alloy_esp32_component auto-detects components
        # from #include statements in source files
    )

    # ESP32-specific compile options
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -mtext-section-literals
        -O2
    )

    # Add linker script from board config
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
