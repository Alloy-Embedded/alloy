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
        _vendor
        _family
        _device
        _arch
        _manifest_mcu
        _manifest_flash_size_bytes
        _manifest_supports_peripheral_examples
        _manifest_supports_uart_logger
        _manifest_supports_dma_probe
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
    set(ALLOY_DEVICE_RUNTIME_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_STARTUP_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_SYSTEM_CLOCK_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_CONNECTORS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_CAPABILITIES_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_SYSTEM_SEQUENCES_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_LOW_POWER_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_INCLUDE_DIRS "" PARENT_SCOPE)
    set(ALLOY_DEVICE_STARTUP_VECTORS_SOURCE "" PARENT_SCOPE)
    set(ALLOY_DEVICE_GENERATED_STARTUP_SOURCE "" PARENT_SCOPE)
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
    set(_runtime_available 0)
    set(_startup_available 0)
    set(_dma_bindings_available 0)
    set(_system_clock_available 0)
    set(_clock_config_available 0)
    set(_connectors_available 0)
    set(_capabilities_available 0)
    set(_interrupt_stubs_available 0)
    set(_system_sequences_available 0)
    set(_low_power_available 0)
    set(_adc_semantics_available 0)
    set(_dac_semantics_available 0)
    set(_can_semantics_available 0)
    set(_rtc_semantics_available 0)
    set(_watchdog_semantics_available 0)
    set(_timer_semantics_available 0)
    set(_pwm_semantics_available 0)
    set(_descriptor_runtime_enabled 0)
    set(_family_root "")
    set(_startup_vectors "")
    set(_generated_startup_source "")

    if(ALLOY_USE_ALLOY_DEVICES AND _vendor AND EXISTS "${_devices_root}")
        set(_family_root "${_devices_root}/${_vendor}/${_family}")
        set(_startup_vectors
            "${_family_root}/generated/devices/${_device}/startup_vectors.cpp"
        )
        set(_generated_startup_source
            "${_family_root}/generated/devices/${_device}/startup.cpp"
        )

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/startup.hpp")
            set(_startup_available 1)
        endif()

        if(
            EXISTS "${_family_root}/generated/runtime/devices/${_device}/dma_bindings.hpp"
            AND EXISTS
                "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/dma.hpp"
        )
            set(_dma_bindings_available 1)
        endif()

        if(
            EXISTS "${_family_root}/generated/runtime/types.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/clock_bindings.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/system_clock.hpp"
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
            set(_runtime_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/system_clock.hpp")
            set(_system_clock_available 1)
        endif()

        if(
            EXISTS "${_family_root}/generated/runtime/devices/${_device}/clock_profiles.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/clock_config.hpp"
        )
            set(_clock_config_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/connectors.hpp")
            set(_connectors_available 1)
        endif()

        if(
            EXISTS "${_family_root}/generated/runtime/devices/${_device}/capabilities.hpp"
            AND EXISTS "${_family_root}/generated/runtime/devices/${_device}/capabilities.json"
        )
            set(_capabilities_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/interrupt_stubs.hpp")
            set(_interrupt_stubs_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/system_sequences.hpp")
            set(_system_sequences_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/low_power.hpp")
            set(_low_power_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/adc.hpp")
            set(_adc_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/dac.hpp")
            set(_dac_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/can.hpp")
            set(_can_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/rtc.hpp")
            set(_rtc_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/watchdog.hpp")
            set(_watchdog_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/timer.hpp")
            set(_timer_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/pwm.hpp")
            set(_pwm_semantics_available 1)
        endif()

        if(_runtime_available)
            set(_contract_available 1)
            set(_descriptor_runtime_enabled 1)
        endif()
    endif()

    set(ALLOY_DEVICE_CONTRACT_AVAILABLE_INT "${_contract_available}")
    set(ALLOY_DEVICE_RUNTIME_AVAILABLE_INT "${_runtime_available}")
    set(ALLOY_DEVICE_STARTUP_AVAILABLE_INT "${_startup_available}")
    set(ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE_INT "${_dma_bindings_available}")
    set(ALLOY_DEVICE_SYSTEM_CLOCK_AVAILABLE_INT "${_system_clock_available}")
    set(ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE_INT "${_clock_config_available}")
    set(ALLOY_DEVICE_CONNECTORS_AVAILABLE_INT "${_connectors_available}")
    set(ALLOY_DEVICE_CAPABILITIES_AVAILABLE_INT "${_capabilities_available}")
    set(ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE_INT "${_interrupt_stubs_available}")
    set(ALLOY_DEVICE_SYSTEM_SEQUENCES_AVAILABLE_INT "${_system_sequences_available}")
    set(ALLOY_DEVICE_LOW_POWER_AVAILABLE_INT "${_low_power_available}")
    set(ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE_INT "${_adc_semantics_available}")
    set(ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE_INT "${_dac_semantics_available}")
    set(ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE_INT "${_can_semantics_available}")
    set(ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE_INT "${_rtc_semantics_available}")
    set(ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE_INT "${_watchdog_semantics_available}")
    set(ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE_INT "${_timer_semantics_available}")
    set(ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE_INT "${_pwm_semantics_available}")
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
    set(ALLOY_DEVICE_GENERATED_STARTUP_SOURCE "${_generated_startup_source}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CONTRACT_AVAILABLE "${_contract_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_RUNTIME_AVAILABLE "${_runtime_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_STARTUP_AVAILABLE "${_startup_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE "${_dma_bindings_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_SYSTEM_CLOCK_AVAILABLE "${_system_clock_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE "${_clock_config_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CONNECTORS_AVAILABLE "${_connectors_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CAPABILITIES_AVAILABLE "${_capabilities_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_INTERRUPT_STUBS_AVAILABLE "${_interrupt_stubs_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_SYSTEM_SEQUENCES_AVAILABLE "${_system_sequences_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_LOW_POWER_AVAILABLE "${_low_power_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE "${_adc_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE "${_dac_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE "${_can_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE "${_rtc_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE "${_watchdog_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE "${_timer_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE "${_pwm_semantics_available}" PARENT_SCOPE)
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
