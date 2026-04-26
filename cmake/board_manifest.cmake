# ==============================================================================
# Board Manifest
# ==============================================================================
#
# Compact board metadata used by the descriptor-driven runtime path. The goal is
# to keep board selection data in one place instead of spreading it across large
# handwritten switches in the top-level build.
#
# Custom-board path:
#   When ALLOY_BOARD is set to "custom", this manifest reads board metadata from a
#   documented set of cache variables instead of looking up an in-tree board:
#
#     required:
#       ALLOY_CUSTOM_BOARD_HEADER     -- absolute path to the consuming project's board.hpp
#       ALLOY_CUSTOM_LINKER_SCRIPT    -- absolute path to the consuming project's linker script
#       ALLOY_DEVICE_VENDOR           -- e.g. "st", "microchip"
#       ALLOY_DEVICE_FAMILY           -- e.g. "stm32g0"
#       ALLOY_DEVICE_NAME             -- e.g. "stm32g071rb" (alloy-devices descriptor name)
#       ALLOY_DEVICE_ARCH             -- one of the values listed in _ALLOY_VALID_ARCHES below
#     optional:
#       ALLOY_DEVICE_MCU              -- canonical part number (cosmetic)
#       ALLOY_FLASH_SIZE_BYTES        -- declared flash size; defaults to 0 = unknown
#
# Refer to docs/CUSTOM_BOARDS.md for the consuming-project recipe.

set(_ALLOY_VALID_ARCHES "cortex-m0plus;cortex-m4;cortex-m7;riscv32;xtensa-lx6;xtensa-lx7;avr;native")

function(_alloy_require_custom_var VAR_NAME)
    if(NOT DEFINED ${VAR_NAME} OR "${${VAR_NAME}}" STREQUAL "")
        message(FATAL_ERROR
            "ALLOY_BOARD=custom requires ${VAR_NAME} to be set before "
            "add_subdirectory(alloy). See docs/CUSTOM_BOARDS.md."
        )
    endif()
endfunction()

function(_alloy_require_absolute_path VAR_NAME)
    if(NOT IS_ABSOLUTE "${${VAR_NAME}}")
        message(FATAL_ERROR
            "${VAR_NAME} must be an absolute path; got: ${${VAR_NAME}}"
        )
    endif()
endfunction()

function(_alloy_validate_custom_arch ARCH)
    list(FIND _ALLOY_VALID_ARCHES "${ARCH}" _idx)
    if(_idx EQUAL -1)
        string(REPLACE ";" ", " _accepted "${_ALLOY_VALID_ARCHES}")
        message(FATAL_ERROR
            "ALLOY_DEVICE_ARCH='${ARCH}' is not accepted; valid values: ${_accepted}"
        )
    endif()
endfunction()

function(
    alloy_resolve_board_manifest
    BOARD_NAME
    OUT_FOUND
    OUT_BOARD_HEADER
    OUT_LINKER_SCRIPT
    OUT_VENDOR
    OUT_FAMILY
    OUT_DEVICE
    OUT_ARCH
    OUT_MCU
    OUT_FLASH_SIZE_BYTES
    OUT_SUPPORTS_PERIPHERAL_EXAMPLES
    OUT_SUPPORTS_UART_LOGGER
    OUT_SUPPORTS_DMA_PROBE
)
    set(_found FALSE)
    set(_board_header "")
    set(_linker_script "")
    set(_vendor "")
    set(_family "")
    set(_device "")
    set(_arch "")
    set(_mcu "")
    set(_flash_size_bytes 0)
    set(_supports_peripheral_examples FALSE)
    set(_supports_uart_logger FALSE)
    set(_supports_dma_probe FALSE)

    if(BOARD_NAME STREQUAL "custom")
        _alloy_require_custom_var(ALLOY_CUSTOM_BOARD_HEADER)
        _alloy_require_custom_var(ALLOY_CUSTOM_LINKER_SCRIPT)
        _alloy_require_custom_var(ALLOY_DEVICE_VENDOR)
        _alloy_require_custom_var(ALLOY_DEVICE_FAMILY)
        _alloy_require_custom_var(ALLOY_DEVICE_NAME)
        _alloy_require_custom_var(ALLOY_DEVICE_ARCH)
        _alloy_require_absolute_path(ALLOY_CUSTOM_BOARD_HEADER)
        _alloy_require_absolute_path(ALLOY_CUSTOM_LINKER_SCRIPT)
        _alloy_validate_custom_arch("${ALLOY_DEVICE_ARCH}")

        if(NOT EXISTS "${ALLOY_CUSTOM_BOARD_HEADER}")
            message(FATAL_ERROR
                "ALLOY_CUSTOM_BOARD_HEADER does not exist: ${ALLOY_CUSTOM_BOARD_HEADER}"
            )
        endif()
        if(NOT EXISTS "${ALLOY_CUSTOM_LINKER_SCRIPT}")
            message(FATAL_ERROR
                "ALLOY_CUSTOM_LINKER_SCRIPT does not exist: ${ALLOY_CUSTOM_LINKER_SCRIPT}"
            )
        endif()

        get_filename_component(_devices_root_abs "${ALLOY_DEVICES_ROOT}" ABSOLUTE)
        set(_descriptor_dir
            "${_devices_root_abs}/${ALLOY_DEVICE_VENDOR}/${ALLOY_DEVICE_FAMILY}/generated/runtime/devices/${ALLOY_DEVICE_NAME}"
        )
        if(NOT IS_DIRECTORY "${_descriptor_dir}")
            message(FATAL_ERROR
                "ALLOY_BOARD=custom: descriptor for "
                "${ALLOY_DEVICE_VENDOR}/${ALLOY_DEVICE_FAMILY}/${ALLOY_DEVICE_NAME} "
                "is missing under ALLOY_DEVICES_ROOT.\n"
                "  Expected: ${_descriptor_dir}\n"
                "  Add support in alloy-devices, or correct the device tuple."
            )
        endif()

        set(_found TRUE)
        set(_board_header "${ALLOY_CUSTOM_BOARD_HEADER}")
        set(_linker_script "${ALLOY_CUSTOM_LINKER_SCRIPT}")
        set(_vendor "${ALLOY_DEVICE_VENDOR}")
        set(_family "${ALLOY_DEVICE_FAMILY}")
        set(_device "${ALLOY_DEVICE_NAME}")
        set(_arch "${ALLOY_DEVICE_ARCH}")
        if(DEFINED ALLOY_DEVICE_MCU AND NOT "${ALLOY_DEVICE_MCU}" STREQUAL "")
            set(_mcu "${ALLOY_DEVICE_MCU}")
        else()
            set(_mcu "${ALLOY_DEVICE_NAME}")
        endif()
        if(DEFINED ALLOY_FLASH_SIZE_BYTES AND NOT "${ALLOY_FLASH_SIZE_BYTES}" STREQUAL "")
            set(_flash_size_bytes "${ALLOY_FLASH_SIZE_BYTES}")
        else()
            set(_flash_size_bytes 0)
        endif()

        if(ALLOY_VERBOSE)
            message(STATUS
                "alloy: custom board resolved -> "
                "${_vendor}/${_family}/${_device} (arch=${_arch}, mcu=${_mcu})"
            )
        endif()
    elseif(BOARD_NAME STREQUAL "host")
        set(_found TRUE)
        set(_board_header "boards/linux_host/board.hpp")
        set(_arch "native")
        set(_mcu "native")
        set(_flash_size_bytes 0)
    elseif(BOARD_NAME STREQUAL "nucleo_g071rb")
        set(_found TRUE)
        set(_board_header "boards/nucleo_g071rb/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/nucleo_g071rb/STM32G071RBT6.ld")
        set(_vendor "st")
        set(_family "stm32g0")
        set(_device "stm32g071rb")
        set(_arch "cortex-m0plus")
        set(_mcu "STM32G071RBT6")
        set(_flash_size_bytes 131072)
        set(_supports_uart_logger TRUE)
        set(_supports_dma_probe TRUE)
    elseif(BOARD_NAME STREQUAL "nucleo_g0b1re")
        set(_found TRUE)
        set(_board_header "boards/nucleo_g0b1re/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/nucleo_g0b1re/STM32G0B1RET6.ld")
        set(_vendor "st")
        set(_family "stm32g0")
        set(_device "stm32g0b1re")
        set(_arch "cortex-m0plus")
        set(_mcu "STM32G0B1RET6")
        set(_flash_size_bytes 524288)
        set(_supports_uart_logger TRUE)
    elseif(BOARD_NAME STREQUAL "nucleo_f401re")
        set(_found TRUE)
        set(_board_header "boards/nucleo_f401re/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/nucleo_f401re/STM32F401RET6.ld")
        set(_vendor "st")
        set(_family "stm32f4")
        set(_device "stm32f401re")
        set(_arch "cortex-m4")
        set(_mcu "STM32F401RET6")
        set(_flash_size_bytes 524288)
        set(_supports_uart_logger TRUE)
        set(_supports_dma_probe TRUE)
    elseif(BOARD_NAME STREQUAL "same70_xpld" OR BOARD_NAME STREQUAL "same70_xplained")
        set(_found TRUE)
        set(_board_header "boards/same70_xplained/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/same70_xplained/ATSAME70Q21.ld")
        set(_vendor "microchip")
        set(_family "same70")
        set(_device "atsame70q21b")
        set(_arch "cortex-m7")
        set(_mcu "ATSAME70Q21B")
        set(_flash_size_bytes 2097152)
        set(_supports_peripheral_examples TRUE)
        set(_supports_uart_logger TRUE)
        set(_supports_dma_probe TRUE)
    elseif(BOARD_NAME STREQUAL "raspberry_pi_pico")
        set(_found TRUE)
        set(_board_header "boards/raspberry_pi_pico/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/raspberry_pi_pico/rp2040.ld")
        set(_vendor "raspberrypi")
        set(_family "rp2040")
        set(_device "pico")
        set(_arch "cortex-m0plus")
        set(_mcu "RP2040")
        set(_flash_size_bytes 2097152)
        set(_supports_uart_logger TRUE)
    elseif(BOARD_NAME STREQUAL "avr128da32_curiosity_nano")
        set(_found TRUE)
        set(_board_header "boards/avr128da32_curiosity_nano/board.hpp")
        set(_vendor "microchip")
        set(_family "avr-da")
        set(_device "avr128da32")
        set(_arch "avr")
        set(_mcu "AVR128DA32")
        set(_flash_size_bytes 131072)
        set(_supports_uart_logger TRUE)
    elseif(BOARD_NAME STREQUAL "esp32c3_devkitm")
        set(_found TRUE)
        set(_board_header "boards/esp32c3_devkitm/board.hpp")
        set(_vendor "espressif")
        set(_family "esp32c3")
        set(_device "esp32c3")
        set(_arch "riscv32")
        set(_mcu "ESP32-C3")
        set(_flash_size_bytes 4194304)
        set(_supports_uart_logger TRUE)
    elseif(BOARD_NAME STREQUAL "esp32s3_devkitc")
        set(_found TRUE)
        set(_board_header "boards/esp32s3_devkitc/board.hpp")
        set(_vendor "espressif")
        set(_family "esp32s3")
        set(_device "esp32s3")
        set(_arch "xtensa-lx7")
        set(_mcu "ESP32-S3")
        set(_flash_size_bytes 8388608)
        set(_supports_uart_logger TRUE)
    elseif(BOARD_NAME STREQUAL "esp32_devkit")
        set(_found TRUE)
        set(_board_header "boards/esp32_devkit/board.hpp")
        set(_vendor "espressif")
        set(_family "esp32")
        set(_device "esp32")
        set(_arch "xtensa-lx6")
        set(_mcu "ESP32")
        set(_flash_size_bytes 4194304)
        set(_supports_uart_logger TRUE)
    elseif(BOARD_NAME STREQUAL "esp_wrover_kit")
        set(_found TRUE)
        set(_board_header "boards/esp_wrover_kit/board.hpp")
        set(_vendor "espressif")
        set(_family "esp32")
        set(_device "esp32-wroom32")
        set(_arch "xtensa-lx6")
        set(_mcu "ESP32-WROVER-B")
        set(_flash_size_bytes 4194304)
        set(_supports_uart_logger TRUE)
    endif()

    set(${OUT_FOUND} "${_found}" PARENT_SCOPE)
    set(${OUT_BOARD_HEADER} "${_board_header}" PARENT_SCOPE)
    set(${OUT_LINKER_SCRIPT} "${_linker_script}" PARENT_SCOPE)
    set(${OUT_VENDOR} "${_vendor}" PARENT_SCOPE)
    set(${OUT_FAMILY} "${_family}" PARENT_SCOPE)
    set(${OUT_DEVICE} "${_device}" PARENT_SCOPE)
    set(${OUT_ARCH} "${_arch}" PARENT_SCOPE)
    set(${OUT_MCU} "${_mcu}" PARENT_SCOPE)
    set(${OUT_FLASH_SIZE_BYTES} "${_flash_size_bytes}" PARENT_SCOPE)
    set(${OUT_SUPPORTS_PERIPHERAL_EXAMPLES} "${_supports_peripheral_examples}" PARENT_SCOPE)
    set(${OUT_SUPPORTS_UART_LOGGER} "${_supports_uart_logger}" PARENT_SCOPE)
    set(${OUT_SUPPORTS_DMA_PROBE} "${_supports_dma_probe}" PARENT_SCOPE)
endfunction()
