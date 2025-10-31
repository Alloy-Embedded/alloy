# Alloy Flash/Upload Targets
#
# Creates targets for flashing firmware to different boards
# using appropriate programming tools (OpenOCD, esptool, etc.)

#
# Create flash target for STM32 boards using OpenOCD
#
function(alloy_add_flash_target_stm32 target_name interface_cfg target_cfg)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Find OpenOCD
    find_program(OPENOCD openocd)

    if(NOT OPENOCD)
        message(STATUS "OpenOCD not found, 'flash' target will not be available")
        message(STATUS "  Install: brew install openocd (macOS) or apt-get install openocd (Linux)")
        return()
    endif()

    # Create flash target
    add_custom_target(flash_${target_name}
        COMMAND ${OPENOCD}
            -f ${interface_cfg}
            -f ${target_cfg}
            -c "program $<TARGET_FILE:${target_name}> verify reset exit"
        DEPENDS ${target_name}
        COMMENT "Flashing ${target_name} to board using OpenOCD"
    )

    message(STATUS "Flash target created: flash_${target_name}")
endfunction()

#
# Create flash target for ESP32 boards using esptool
#
function(alloy_add_flash_target_esp32 target_name port baud)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Find esptool
    find_program(ESPTOOL esptool.py esptool)

    if(NOT ESPTOOL)
        message(STATUS "esptool.py not found, 'flash' target will not be available")
        message(STATUS "  Install: pip install esptool")
        return()
    endif()

    # Create binary output
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${target_name}.bin
        COMMENT "Creating binary file for ${target_name}"
    )

    # Create flash target
    add_custom_target(flash_${target_name}
        COMMAND ${ESPTOOL}
            --chip esp32
            --port ${port}
            --baud ${baud}
            write_flash 0x1000 ${target_name}.bin
        DEPENDS ${target_name}
        COMMENT "Flashing ${target_name} to ESP32"
    )

    message(STATUS "Flash target created: flash_${target_name} (port: ${port}, baud: ${baud})")
endfunction()

#
# Create flash target for RP2040 boards using picotool
#
function(alloy_add_flash_target_rp2040 target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Find picotool
    find_program(PICOTOOL picotool)

    # Create UF2 output
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${target_name}.bin
        COMMENT "Creating binary file for ${target_name}"
    )

    if(PICOTOOL)
        # If picotool is available, create automated flash target
        add_custom_target(flash_${target_name}
            COMMAND ${PICOTOOL} load -v -x $<TARGET_FILE:${target_name}>
            DEPENDS ${target_name}
            COMMENT "Flashing ${target_name} to RP2040 using picotool"
        )
        message(STATUS "Flash target created: flash_${target_name} (using picotool)")
    else()
        # If picotool not available, provide manual instructions
        add_custom_target(flash_${target_name}
            COMMAND ${CMAKE_COMMAND} -E echo "========================================="
            COMMAND ${CMAKE_COMMAND} -E echo "Manual flash instructions for RP2040:"
            COMMAND ${CMAKE_COMMAND} -E echo "1. Hold BOOTSEL button while connecting USB"
            COMMAND ${CMAKE_COMMAND} -E echo "2. Copy ${target_name}.bin to RPI-RP2 drive"
            COMMAND ${CMAKE_COMMAND} -E echo "========================================="
            DEPENDS ${target_name}
            COMMENT "Manual flash instructions for ${target_name}"
        )
        message(STATUS "Flash target created: flash_${target_name} (manual instructions)")
        message(STATUS "  Install picotool for automated flashing: https://github.com/raspberrypi/picotool")
    endif()
endfunction()

#
# Create flash target for ATSAMD21 boards using BOSSA
#
function(alloy_add_flash_target_samd21 target_name port)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Find bossac
    find_program(BOSSAC bossac)

    if(NOT BOSSAC)
        message(STATUS "bossac not found, 'flash' target will not be available")
        message(STATUS "  Install: brew install bossa (macOS) or apt-get install bossa-cli (Linux)")
        return()
    endif()

    # Create binary output
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${target_name}.bin
        COMMENT "Creating binary file for ${target_name}"
    )

    # Create flash target
    add_custom_target(flash_${target_name}
        COMMAND ${BOSSAC}
            --port=${port}
            --erase
            --write
            --verify
            --reset
            ${target_name}.bin
        DEPENDS ${target_name}
        COMMENT "Flashing ${target_name} to SAMD21 using BOSSA"
    )

    message(STATUS "Flash target created: flash_${target_name} (port: ${port})")
endfunction()

#
# Auto-detect and create appropriate flash target based on board
#
# Usage:
#   alloy_add_flash_target(my_firmware)
#
# Optional arguments:
#   PORT      - Serial port (default: auto-detect)
#   BAUD      - Baud rate for serial upload (default: 115200)
#   INTERFACE - OpenOCD interface config (default: stlink-v2.cfg)
#   TARGET    - OpenOCD target config (default: auto-detect)
#
function(alloy_add_flash_target target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Parse optional arguments
    set(options "")
    set(oneValueArgs PORT BAUD INTERFACE TARGET)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Default values
    if(NOT ARG_PORT)
        set(ARG_PORT "/dev/ttyUSB0")
    endif()

    if(NOT ARG_BAUD)
        set(ARG_BAUD 115200)
    endif()

    if(NOT ARG_INTERFACE)
        set(ARG_INTERFACE "interface/stlink-v2.cfg")
    endif()

    # Detect board and create appropriate flash target
    if(ALLOY_BOARD STREQUAL "bluepill")
        # STM32F103C8 Blue Pill
        if(NOT ARG_TARGET)
            set(ARG_TARGET "target/stm32f1x.cfg")
        endif()
        alloy_add_flash_target_stm32(${target_name} ${ARG_INTERFACE} ${ARG_TARGET})

    elseif(ALLOY_BOARD STREQUAL "stm32f407vg")
        # STM32F407VG Discovery
        if(NOT ARG_TARGET)
            set(ARG_TARGET "target/stm32f4x.cfg")
        endif()
        alloy_add_flash_target_stm32(${target_name} ${ARG_INTERFACE} ${ARG_TARGET})

    elseif(ALLOY_BOARD STREQUAL "esp32_devkit")
        # ESP32 DevKit
        alloy_add_flash_target_esp32(${target_name} ${ARG_PORT} ${ARG_BAUD})

    elseif(ALLOY_BOARD STREQUAL "rp_pico")
        # Raspberry Pi Pico
        alloy_add_flash_target_rp2040(${target_name})

    elseif(ALLOY_BOARD STREQUAL "arduino_zero")
        # Arduino Zero (ATSAMD21)
        alloy_add_flash_target_samd21(${target_name} ${ARG_PORT})

    elseif(ALLOY_BOARD STREQUAL "host")
        # Host board doesn't need flash target
        message(STATUS "Host board selected, no flash target needed")

    else()
        message(WARNING "Unknown board '${ALLOY_BOARD}', cannot create flash target")
    endif()

    # Create convenient "flash" alias if this is the only target
    if(NOT TARGET flash AND TARGET flash_${target_name})
        add_custom_target(flash DEPENDS flash_${target_name})
        message(STATUS "Flash alias created: 'flash' -> 'flash_${target_name}'")
    endif()
endfunction()

#
# Create binary and hex outputs for firmware
#
function(alloy_add_binary_outputs target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Skip for host builds
    if(ALLOY_BOARD STREQUAL "host")
        return()
    endif()

    # Create .bin file
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${target_name}.bin
        COMMENT "Creating binary file: ${target_name}.bin"
    )

    # Create .hex file
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target_name}> ${target_name}.hex
        COMMENT "Creating hex file: ${target_name}.hex"
    )

    # Print size information
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${target_name}>
        COMMENT "Firmware size:"
    )

    message(STATUS "Binary outputs will be created for ${target_name}: .bin, .hex")
endfunction()
