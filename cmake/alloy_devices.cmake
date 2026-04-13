# ==============================================================================
# alloy-devices Integration
# ==============================================================================
#
# Resolves the selected board to a published descriptor tree in alloy-devices and
# generates the stable import header consumed by src/device/selected.hpp.
#

set(
    ALLOY_DEVICES_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../alloy-devices"
    CACHE PATH
    "Path to the alloy-devices checkout"
)

set(ALLOY_DEVICES_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(
    ALLOY_USE_ALLOY_DEVICES
    "Enable descriptor-driven device import from alloy-devices"
    ON
)

function(alloy_map_board_to_device BOARD_NAME OUT_VENDOR OUT_FAMILY OUT_DEVICE OUT_ARCH)
    alloy_resolve_board_manifest(
        "${BOARD_NAME}"
        _manifest_found
        _manifest_board_header
        _manifest_linker_script
        _manifest_legacy_startup
        _vendor
        _family
        _device
        _arch
        _manifest_mcu
        _manifest_flash_size_bytes
        _manifest_supports_peripheral_examples
        _manifest_supports_uart_logger
    )

    if(NOT _manifest_found)
        set(_vendor "")
        set(_family "")
        set(_device "")
        set(_arch "")
    endif()

    set(${OUT_VENDOR} "${_vendor}" PARENT_SCOPE)
    set(${OUT_FAMILY} "${_family}" PARENT_SCOPE)
    set(${OUT_DEVICE} "${_device}" PARENT_SCOPE)
    set(${OUT_ARCH} "${_arch}" PARENT_SCOPE)
endfunction()

function(alloy_configure_selected_device)
    set(ALLOY_DEVICE_CONTRACT_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_RUNTIME_LITE_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_INCLUDE_DIRS "" PARENT_SCOPE)
    set(ALLOY_DEVICE_STARTUP_VECTORS_SOURCE "" PARENT_SCOPE)
    set(ALLOY_DESCRIPTOR_RUNTIME_ENABLED FALSE PARENT_SCOPE)

    get_filename_component(_devices_root "${ALLOY_DEVICES_ROOT}" ABSOLUTE)
    set(ALLOY_DEVICES_ROOT_ABS "${_devices_root}" PARENT_SCOPE)

    alloy_map_board_to_device(
        "${ALLOY_BOARD}"
        _vendor
        _family
        _device
        _arch
    )

    set(ALLOY_DEVICE_VENDOR "${_vendor}" PARENT_SCOPE)
    set(ALLOY_DEVICE_FAMILY "${_family}" PARENT_SCOPE)
    set(ALLOY_DEVICE_NAME "${_device}" PARENT_SCOPE)
    set(ALLOY_DEVICE_ARCH "${_arch}" PARENT_SCOPE)

    set(_selected_config_dir "${CMAKE_BINARY_DIR}/generated/alloy/device")
    file(MAKE_DIRECTORY "${_selected_config_dir}")

    set(_selected_config_header "${_selected_config_dir}/selected_config.hpp")

    set(_contract_available 0)
    set(_runtime_lite_available 0)
    set(_descriptor_runtime_enabled 0)
    set(_family_root "")
    set(_startup_vectors "")

    if(ALLOY_USE_ALLOY_DEVICES AND _vendor AND EXISTS "${_devices_root}")
        set(_family_root "${_devices_root}/${_vendor}/${_family}")
        set(_startup_vectors
            "${_family_root}/generated/devices/${_device}/startup_vectors.cpp"
        )

        if(
            EXISTS "${_family_root}/generated/runtime/types.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/clock_bindings.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/common.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/gpio.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/i2c.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/spi.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/uart.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/peripheral_instances.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/pins.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/register_fields.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/registers.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/routes.hpp"
        )
            set(_runtime_lite_available 1)
        endif()

        if(_runtime_lite_available)
            set(_contract_available 1)
            set(_descriptor_runtime_enabled 1)
        endif()
    endif()

    set(ALLOY_DEVICE_CONTRACT_AVAILABLE_INT "${_contract_available}")
    set(ALLOY_DEVICE_RUNTIME_LITE_AVAILABLE_INT "${_runtime_lite_available}")
    set(ALLOY_DEVICE_SELECTED_VENDOR "${_vendor}")
    set(ALLOY_DEVICE_SELECTED_FAMILY "${_family}")
    set(ALLOY_DEVICE_SELECTED_NAME "${_device}")
    set(ALLOY_DEVICE_SELECTED_ARCH "${_arch}")

    configure_file(
        "${ALLOY_DEVICES_MODULE_DIR}/templates/selected_config.hpp.in"
        "${_selected_config_header}"
        @ONLY
    )

    set(
        ALLOY_DEVICE_INCLUDE_DIRS
        "${_devices_root};${CMAKE_BINARY_DIR}/generated"
        PARENT_SCOPE
    )
    set(ALLOY_DEVICE_SELECTED_CONFIG_HEADER "${_selected_config_header}" PARENT_SCOPE)
    set(ALLOY_DEVICE_FAMILY_ROOT "${_family_root}" PARENT_SCOPE)
    set(ALLOY_DEVICE_STARTUP_VECTORS_SOURCE "${_startup_vectors}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CONTRACT_AVAILABLE "${_contract_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_RUNTIME_LITE_AVAILABLE "${_runtime_lite_available}" PARENT_SCOPE)
    set(ALLOY_DESCRIPTOR_RUNTIME_ENABLED "${_descriptor_runtime_enabled}" PARENT_SCOPE)

    if(_device)
        set(ALLOY_MCU "${_device}" PARENT_SCOPE)
    endif()

    if(_arch)
        if(_arch STREQUAL "native")
            set(ALLOY_ARCH "native" PARENT_SCOPE)
        else()
            set(ALLOY_ARCH "arm-${_arch}" PARENT_SCOPE)
        endif()
    endif()
endfunction()
