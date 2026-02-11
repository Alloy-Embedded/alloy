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

# Load generated board metadata mapping if available.
set(_MICROCORE_GENERATED_BOARD_METADATA_FILE "${CMAKE_CURRENT_LIST_DIR}/generated/board_metadata.cmake")
if(NOT DEFINED MICROCORE_GENERATED_BOARD_PLATFORM_MAP AND EXISTS "${_MICROCORE_GENERATED_BOARD_METADATA_FILE}")
    include("${_MICROCORE_GENERATED_BOARD_METADATA_FILE}")
endif()

#
# Auto-detect and validate toolchain based on board
#
function(alloy_validate_toolchain)
    if(NOT DEFINED MICROCORE_BOARD)
        message(STATUS "No board specified, skipping toolchain validation")
        return()
    endif()

    set(_alloy_toolchain_board_map)
    if(DEFINED MICROCORE_GENERATED_BOARD_PLATFORM_MAP)
        list(APPEND _alloy_toolchain_board_map ${MICROCORE_GENERATED_BOARD_PLATFORM_MAP})
    endif()
    list(APPEND _alloy_toolchain_board_map
        "nucleo_f401re:stm32f4"
        "nucleo_f722ze:stm32f7"
        "nucleo_g071rb:stm32g0"
        "nucleo_g0b1re:stm32g0"
        "same70_xplained:same70"
        "same70_xpld:same70"
        "bluepill:stm32f1"
        "stm32f407vg:stm32f4"
        "esp32_devkit:esp32"
        "arduino_zero:samd21"
        "rp_pico:rp2040"
        "rp2040_zero:rp2040"
        "host:linux"
    )
    list(REMOVE_DUPLICATES _alloy_toolchain_board_map)

    set(_toolchain_platform "")
    foreach(mapping ${_alloy_toolchain_board_map})
        string(REPLACE ":" ";" mapping_parts "${mapping}")
        list(LENGTH mapping_parts mapping_parts_len)
        if(mapping_parts_len GREATER 1)
            list(GET mapping_parts 0 mapping_board)
            list(GET mapping_parts 1 mapping_platform)
            if("${mapping_board}" STREQUAL "${MICROCORE_BOARD}")
                set(_toolchain_platform "${mapping_platform}")
                break()
            endif()
        endif()
    endforeach()

    if("${_toolchain_platform}" STREQUAL "linux")
        message(STATUS "Host board selected, using native compiler")
        return()
    endif()

    if("${_toolchain_platform}" STREQUAL "esp32")
        alloy_validate_xtensa_toolchain()
    elseif(NOT "${_toolchain_platform}" STREQUAL "")
        alloy_validate_arm_toolchain()
    else()
        message(WARNING "Unknown board '${MICROCORE_BOARD}', cannot validate toolchain")
    endif()
endfunction()
