# Alloy Toolchain Detection and Validation
#
# Checks if the required cross-compilation toolchain is installed
# and provides helpful error messages if not found.

#
# Check if a command exists in PATH
#
function(alloy_check_command command_name out_found out_version)
    # Try to find the command
    find_program(COMMAND_PATH ${command_name})

    if(COMMAND_PATH)
        # Try to get version
        execute_process(
            COMMAND ${command_name} --version
            OUTPUT_VARIABLE version_output
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Extract first line of version output
        string(REGEX MATCH "^[^\n]+" first_line "${version_output}")

        set(${out_found} TRUE PARENT_SCOPE)
        set(${out_version} "${first_line}" PARENT_SCOPE)
    else()
        set(${out_found} FALSE PARENT_SCOPE)
        set(${out_version} "" PARENT_SCOPE)
    endif()

    unset(COMMAND_PATH CACHE)
endfunction()

#
# Validate ARM toolchain (arm-none-eabi-gcc)
#
function(alloy_validate_arm_toolchain)
    alloy_check_command(arm-none-eabi-gcc found version)

    if(found)
        message(STATUS "ARM toolchain found: ${version}")
    else()
        message(FATAL_ERROR
            "ARM toolchain not found!\n"
            "\n"
            "  Required: arm-none-eabi-gcc\n"
            "\n"
            "  Install instructions:\n"
            "    macOS:   brew install arm-none-eabi-gcc\n"
            "    Ubuntu:  sudo apt-get install gcc-arm-none-eabi\n"
            "    Windows: Download from https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm\n"
        )
    endif()
endfunction()

#
# Validate Xtensa ESP32 toolchain (xtensa-esp32-elf-gcc)
#
function(alloy_validate_xtensa_toolchain)
    alloy_check_command(xtensa-esp32-elf-gcc found version)

    if(found)
        message(STATUS "Xtensa ESP32 toolchain found: ${version}")
    else()
        message(FATAL_ERROR
            "Xtensa ESP32 toolchain not found!\n"
            "\n"
            "  Required: xtensa-esp32-elf-gcc\n"
            "\n"
            "  Install instructions:\n"
            "    Install ESP-IDF framework which includes the toolchain:\n"
            "    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/\n"
            "\n"
            "    Or download standalone toolchain:\n"
            "    https://github.com/espressif/crosstool-NG/releases\n"
        )
    endif()
endfunction()

#
# Auto-detect and validate toolchain based on board
#
function(alloy_validate_toolchain)
    if(NOT DEFINED ALLOY_BOARD)
        message(STATUS "No board specified, skipping toolchain validation")
        return()
    endif()

    # Host board doesn't need cross-compilation toolchain
    if(ALLOY_BOARD STREQUAL "host")
        message(STATUS "Host board selected, using native compiler")
        return()
    endif()

    # Determine required toolchain based on board
    if(ALLOY_BOARD STREQUAL "esp32_devkit")
        alloy_validate_xtensa_toolchain()
    elseif(ALLOY_BOARD MATCHES "^(bluepill|stm32f407vg|arduino_zero|rp_pico)$")
        alloy_validate_arm_toolchain()
    else()
        message(WARNING "Unknown board '${ALLOY_BOARD}', cannot validate toolchain")
    endif()
endfunction()
