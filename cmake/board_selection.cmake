# ==============================================================================
# Board Selection Module
# ==============================================================================
#
# This module provides board-specific configuration for Alloy HAL.
# Boards are physical hardware (e.g., SAME70-Xplained, STM32F4-Discovery).
# Each board maps to a platform and defines peripheral type aliases.
#
# Board vs Platform:
#   - Platform: HAL implementation (same70, stm32f4, linux, esp32)
#   - Board: Physical hardware with specific pin mappings
#
# Example:
#   - Board: "same70_xplained" → Platform: "same70"
#   - Board defines: led_green = GpioPin<PIOC, 8>, uart_console = Uart0, etc.
#
# Usage:
#   cmake -DMICROCORE_BOARD=same70_xplained -B build
#
# @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
# ==============================================================================

# ------------------------------------------------------------------------------
# Board to Platform Mapping
# ------------------------------------------------------------------------------

# This function maps board names to platforms
# It's used when MICROCORE_PLATFORM is not explicitly set
function(alloy_board_to_platform BOARD_NAME OUT_PLATFORM)
    # Map board names to platforms
    if(BOARD_NAME STREQUAL "same70_xplained" OR BOARD_NAME STREQUAL "same70_xpld")
        set(${OUT_PLATFORM} "same70" PARENT_SCOPE)

    elseif(BOARD_NAME STREQUAL "stm32f407_discovery" OR BOARD_NAME STREQUAL "stm32f407vg")
        set(${OUT_PLATFORM} "stm32f4" PARENT_SCOPE)

    elseif(BOARD_NAME STREQUAL "bluepill" OR BOARD_NAME STREQUAL "stm32f103c8")
        set(${OUT_PLATFORM} "stm32f1" PARENT_SCOPE)

    elseif(BOARD_NAME MATCHES "^esp32")
        set(${OUT_PLATFORM} "esp32" PARENT_SCOPE)

    elseif(BOARD_NAME STREQUAL "arduino_zero" OR BOARD_NAME MATCHES "samd21")
        set(${OUT_PLATFORM} "samd21" PARENT_SCOPE)

    elseif(BOARD_NAME MATCHES "rp2040" OR BOARD_NAME MATCHES "rp_pico")
        set(${OUT_PLATFORM} "rp2040" PARENT_SCOPE)

    elseif(BOARD_NAME STREQUAL "nucleo_g0b1re" OR BOARD_NAME MATCHES "stm32g0")
        set(${OUT_PLATFORM} "stm32g0" PARENT_SCOPE)

    elseif(BOARD_NAME STREQUAL "host")
        set(${OUT_PLATFORM} "linux" PARENT_SCOPE)

    else()
        # Unknown board - fallback to linux for testing
        message(WARNING "Unknown board '${BOARD_NAME}', using 'linux' platform")
        set(${OUT_PLATFORM} "linux" PARENT_SCOPE)
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Board Configuration Files
# ------------------------------------------------------------------------------

# Board configuration files define:
#   1. Platform mapping (which platform this board uses)
#   2. Peripheral type aliases (board::led_green, board::uart0, etc.)
#   3. Pin mappings and hardware-specific constants
#
# Location: boards/<board_name>/board.hpp
#
# Example (boards/same70_xplained/board.hpp):
#   namespace board {
#       using uart0 = hal::same70::Uart<UART0_BASE, ID_UART0>;
#       using led_green = hal::same70::GpioPin<PIOC_BASE, 8>;
#   }

# Check if board configuration directory exists
if(DEFINED MICROCORE_BOARD)
    set(MICROCORE_BOARD_DIR "${CMAKE_SOURCE_DIR}/boards/${MICROCORE_BOARD}")

    if(EXISTS "${MICROCORE_BOARD_DIR}")
        message(STATUS "Board directory found: ${MICROCORE_BOARD_DIR}")

        # Check for board.hpp header
        if(EXISTS "${MICROCORE_BOARD_DIR}/board.hpp")
            message(STATUS "  Board header: ${MICROCORE_BOARD_DIR}/board.hpp")
        else()
            message(STATUS "  Board header: NOT FOUND (board.hpp missing)")
        endif()

        # Check for board.cmake (optional board-specific CMake config)
        if(EXISTS "${MICROCORE_BOARD_DIR}/board.cmake")
            message(STATUS "  Board CMake: ${MICROCORE_BOARD_DIR}/board.cmake")
            include("${MICROCORE_BOARD_DIR}/board.cmake")
        endif()

        # Export board directory for use by applications
        set(MICROCORE_BOARD_DIR ${MICROCORE_BOARD_DIR} PARENT_SCOPE)

    else()
        message(STATUS "Board directory not found: ${MICROCORE_BOARD_DIR}")
        message(STATUS "  Board configuration will use platform defaults")
    endif()
endif()

# ------------------------------------------------------------------------------
# Board Include Path
# ------------------------------------------------------------------------------

# Add board directory to include path so applications can:
#   #include "board.hpp"
#
# This allows platform-agnostic code like:
#   auto led = board::led_green{};
#   auto uart = board::uart_console{};

if(DEFINED MICROCORE_BOARD_DIR AND EXISTS "${MICROCORE_BOARD_DIR}")
    # Add to global include directories
    include_directories(${MICROCORE_BOARD_DIR})

    message(STATUS "Added board include path: ${MICROCORE_BOARD_DIR}")
endif()

# ------------------------------------------------------------------------------
# Board Validation
# ------------------------------------------------------------------------------

# Verify board is compatible with selected platform
if(DEFINED MICROCORE_BOARD AND DEFINED MICROCORE_PLATFORM)
    # Get expected platform for this board
    alloy_board_to_platform(${MICROCORE_BOARD} EXPECTED_PLATFORM)

    # Check if platform matches (some boards support multiple platforms)
    if(NOT "${MICROCORE_PLATFORM}" STREQUAL "${EXPECTED_PLATFORM}")
        message(WARNING
            "Platform mismatch detected:\n"
            "  Board '${MICROCORE_BOARD}' typically uses platform '${EXPECTED_PLATFORM}'\n"
            "  But MICROCORE_PLATFORM is set to '${MICROCORE_PLATFORM}'\n"
            "This may work if the board supports multiple platforms, but verify carefully."
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Board Information Summary
# ------------------------------------------------------------------------------

if(DEFINED MICROCORE_BOARD)
    message(STATUS "------------------------------------------------------------------------------")
    message(STATUS "Board Configuration: ${MICROCORE_BOARD}")
    message(STATUS "  Platform: ${MICROCORE_PLATFORM}")
    if(DEFINED MICROCORE_BOARD_DIR)
        message(STATUS "  Location: ${MICROCORE_BOARD_DIR}")
    endif()
    message(STATUS "------------------------------------------------------------------------------")
endif()
