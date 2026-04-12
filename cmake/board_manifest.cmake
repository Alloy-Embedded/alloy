# ==============================================================================
# Board Manifest
# ==============================================================================
#
# Compact board metadata used by the descriptor-driven runtime path. The goal is
# to keep board selection data in one place instead of spreading it across large
# handwritten switches in the top-level build.
#

function(
    alloy_resolve_board_manifest
    BOARD_NAME
    OUT_FOUND
    OUT_BOARD_HEADER
    OUT_LINKER_SCRIPT
    OUT_LEGACY_STARTUP
    OUT_VENDOR
    OUT_FAMILY
    OUT_DEVICE
    OUT_ARCH
    OUT_MCU
    OUT_FLASH_SIZE_BYTES
    OUT_SUPPORTS_PERIPHERAL_EXAMPLES
    OUT_SUPPORTS_UART_LOGGER
)
    set(_found FALSE)
    set(_board_header "")
    set(_linker_script "")
    set(_legacy_startup "")
    set(_vendor "")
    set(_family "")
    set(_device "")
    set(_arch "")
    set(_mcu "")
    set(_flash_size_bytes 0)
    set(_supports_peripheral_examples FALSE)
    set(_supports_uart_logger FALSE)

    if(BOARD_NAME STREQUAL "host")
        set(_found TRUE)
        set(_board_header "boards/linux_host/board.hpp")
        set(_arch "native")
        set(_mcu "native")
        set(_flash_size_bytes 0)
    elseif(BOARD_NAME STREQUAL "nucleo_g071rb")
        set(_found TRUE)
        set(_board_header "boards/nucleo_g071rb/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/nucleo_g071rb/STM32G071RBT6.ld")
        set(_legacy_startup "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp")
        set(_vendor "st")
        set(_family "stm32g0")
        set(_device "stm32g071rb")
        set(_arch "cortex-m0plus")
        set(_mcu "STM32G071RBT6")
        set(_flash_size_bytes 131072)
    elseif(BOARD_NAME STREQUAL "nucleo_g0b1re")
        set(_found TRUE)
        set(_board_header "boards/nucleo_g0b1re/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/nucleo_g0b1re/STM32G0B1RET6.ld")
        set(_legacy_startup "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp")
        set(_vendor "st")
        set(_family "stm32g0")
        set(_device "stm32g0b1re")
        set(_arch "cortex-m0plus")
        set(_mcu "STM32G0B1RET6")
        set(_flash_size_bytes 524288)
    elseif(BOARD_NAME STREQUAL "nucleo_f401re")
        set(_found TRUE)
        set(_board_header "boards/nucleo_f401re/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/nucleo_f401re/STM32F401RET6.ld")
        set(_legacy_startup "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32f4/stm32f401/startup.cpp")
        set(_vendor "st")
        set(_family "stm32f4")
        set(_device "stm32f401re")
        set(_arch "cortex-m4")
        set(_mcu "STM32F401RET6")
        set(_flash_size_bytes 524288)
        set(_supports_peripheral_examples TRUE)
    elseif(BOARD_NAME STREQUAL "same70_xpld" OR BOARD_NAME STREQUAL "same70_xplained")
        set(_found TRUE)
        set(_board_header "boards/same70_xplained/board.hpp")
        set(_linker_script "${CMAKE_SOURCE_DIR}/boards/same70_xplained/ATSAME70Q21.ld")
        set(_legacy_startup "${CMAKE_SOURCE_DIR}/src/hal/vendors/atmel/same70/startup_same70.cpp")
        set(_vendor "microchip")
        set(_family "same70")
        set(_device "atsame70q21b")
        set(_arch "cortex-m7")
        set(_mcu "ATSAME70Q21B")
        set(_flash_size_bytes 2097152)
        set(_supports_peripheral_examples TRUE)
        set(_supports_uart_logger TRUE)
    endif()

    set(${OUT_FOUND} "${_found}" PARENT_SCOPE)
    set(${OUT_BOARD_HEADER} "${_board_header}" PARENT_SCOPE)
    set(${OUT_LINKER_SCRIPT} "${_linker_script}" PARENT_SCOPE)
    set(${OUT_LEGACY_STARTUP} "${_legacy_startup}" PARENT_SCOPE)
    set(${OUT_VENDOR} "${_vendor}" PARENT_SCOPE)
    set(${OUT_FAMILY} "${_family}" PARENT_SCOPE)
    set(${OUT_DEVICE} "${_device}" PARENT_SCOPE)
    set(${OUT_ARCH} "${_arch}" PARENT_SCOPE)
    set(${OUT_MCU} "${_mcu}" PARENT_SCOPE)
    set(${OUT_FLASH_SIZE_BYTES} "${_flash_size_bytes}" PARENT_SCOPE)
    set(${OUT_SUPPORTS_PERIPHERAL_EXAMPLES} "${_supports_peripheral_examples}" PARENT_SCOPE)
    set(${OUT_SUPPORTS_UART_LOGGER} "${_supports_uart_logger}" PARENT_SCOPE)
endfunction()
