# ==============================================================================
# alloy-devices Integration
# ==============================================================================
#
# Two resolution modes — tried in order:
#
#   Mode B (local checkout):
#     Set ALLOY_DEVICES_ROOT to a directory that exists.
#     The variable defaults to ../alloy-devices (sibling repo), which is the
#     developer workflow.  If the directory does not exist, Mode A is tried.
#
#   Mode A (package registry / cache):
#     Device packages are downloaded from GitHub Releases and unpacked to a
#     local cache directory.  Cache resolution order:
#       1. ALLOY_DEVICE_CACHE_DIR cmake variable
#       2. ALLOY_DEVICE_CACHE environment variable
#       3. ~/.alloy/devices/   (platform home)
#       4. ${CMAKE_BINARY_DIR}/_alloy_devices_cache  (build-local fallback)
#
#   Offline mode (ALLOY_OFFLINE=ON):
#     Disables all network downloads.  Fails with a clear error if the device
#     package is not already in the cache.
#

include("${CMAKE_CURRENT_LIST_DIR}/alloy_devices_version.cmake")

set(
    ALLOY_DEVICES_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../alloy-devices"
    CACHE PATH
    "Path to a local alloy-devices checkout (Mode B). Leave at default to use the package registry."
)

set(ALLOY_DEVICE_CACHE_DIR "" CACHE PATH
    "Override alloy-devices package cache directory (Mode A). Empty = auto-resolve.")
mark_as_advanced(ALLOY_DEVICE_CACHE_DIR)

option(ALLOY_OFFLINE "Disable network downloads; fail if device package not in cache" OFF)

# ------------------------------------------------------------------------------
# _alloy_resolve_device_cache_dir(<out_var>)
# Returns the resolved device cache directory path (does not create it).
# ------------------------------------------------------------------------------
function(_alloy_resolve_device_cache_dir OUT_VAR)
    if(ALLOY_DEVICE_CACHE_DIR AND NOT "${ALLOY_DEVICE_CACHE_DIR}" STREQUAL "")
        set(${OUT_VAR} "${ALLOY_DEVICE_CACHE_DIR}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{ALLOY_DEVICE_CACHE} AND NOT "$ENV{ALLOY_DEVICE_CACHE}" STREQUAL "")
        set(${OUT_VAR} "$ENV{ALLOY_DEVICE_CACHE}" PARENT_SCOPE)
        return()
    endif()
    if(CMAKE_HOST_WIN32)
        set(_home_env "$ENV{USERPROFILE}")
    else()
        set(_home_env "$ENV{HOME}")
    endif()
    if(_home_env)
        set(${OUT_VAR} "${_home_env}/.alloy/devices" PARENT_SCOPE)
        return()
    endif()
    set(${OUT_VAR} "${CMAKE_BINARY_DIR}/_alloy_devices_cache" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# _alloy_ensure_device_package(<vendor> <family>)
# Ensures the device package is unpacked in the cache directory.
# Downloads from the registry if not present and ALLOY_OFFLINE is OFF.
# ------------------------------------------------------------------------------
function(_alloy_ensure_device_package vendor family)
    _alloy_resolve_device_cache_dir(_cache_dir)

    set(_sentinel "${_cache_dir}/${vendor}/${family}/generated/runtime/types.hpp")
    if(EXISTS "${_sentinel}")
        return()  # already cached and extracted
    endif()

    # Offline: cannot download
    if(ALLOY_OFFLINE)
        message(FATAL_ERROR
            "alloy-devices: package '${vendor}-${family}' not in cache at '${_cache_dir}'.\n"
            "  ALLOY_OFFLINE=ON — network downloads are disabled.\n"
            "  Prefetch the package with:\n"
            "    python scripts/alloyctl.py device prefetch --family ${family}\n"
            "  Or point to a local checkout:\n"
            "    cmake -DALLOY_DEVICES_ROOT=/path/to/alloy-devices ...")
    endif()

    set(_pkg_name "${vendor}-${family}-${ALLOY_DEVICES_VERSION}.tar.gz")
    set(_pkg_url  "${ALLOY_DEVICES_BASE_URL}/${_pkg_name}")
    set(_pkg_dest "${_cache_dir}/${_pkg_name}")

    file(MAKE_DIRECTORY "${_cache_dir}")

    # -- Step 1: fetch checksums.json for SHA256 verification ------------------
    set(_chk_dest "${_cache_dir}/checksums-${ALLOY_DEVICES_VERSION}.json")
    set(_expected_hash "")

    if(NOT EXISTS "${_chk_dest}")
        file(DOWNLOAD "${ALLOY_DEVICES_BASE_URL}/checksums.json" "${_chk_dest}"
            TIMEOUT 30 STATUS _chk_status QUIET)
        list(GET _chk_status 0 _chk_code)
        if(NOT _chk_code EQUAL 0)
            message(WARNING
                "alloy-devices: could not fetch checksums.json — SHA256 verification skipped.\n"
                "  Status: ${_chk_status}")
            file(REMOVE "${_chk_dest}")
        endif()
    endif()

    if(EXISTS "${_chk_dest}")
        file(READ "${_chk_dest}" _chk_content)
        string(JSON _sha256_raw ERROR_VARIABLE _json_err
            GET "${_chk_content}" "${_pkg_name}")
        if(NOT _json_err)
            string(REGEX REPLACE "^sha256:" "" _expected_hash "${_sha256_raw}")
        endif()
    endif()

    # -- Step 2: download package tar.gz ---------------------------------------
    if(NOT EXISTS "${_pkg_dest}")
        message(STATUS "alloy-devices: downloading ${_pkg_name} from ${ALLOY_DEVICES_BASE_URL}...")
        if(_expected_hash)
            file(DOWNLOAD "${_pkg_url}" "${_pkg_dest}"
                TIMEOUT 300 SHOW_PROGRESS
                EXPECTED_HASH "SHA256=${_expected_hash}"
                STATUS _dl_status)
        else()
            file(DOWNLOAD "${_pkg_url}" "${_pkg_dest}"
                TIMEOUT 300 SHOW_PROGRESS
                STATUS _dl_status)
        endif()

        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            file(REMOVE "${_pkg_dest}")
            message(FATAL_ERROR
                "alloy-devices: download failed for '${_pkg_url}'.\n"
                "  Error: ${_dl_status}\n"
                "  Check your network.  To use a local checkout instead:\n"
                "    cmake -DALLOY_DEVICES_ROOT=/path/to/alloy-devices ...")
        endif()
    endif()

    # -- Step 3: extract -------------------------------------------------------
    message(STATUS "alloy-devices: extracting ${_pkg_name} -> ${_cache_dir}")
    file(ARCHIVE_EXTRACT INPUT "${_pkg_dest}" DESTINATION "${_cache_dir}")

    if(NOT EXISTS "${_sentinel}")
        message(FATAL_ERROR
            "alloy-devices: extraction completed but expected file not found:\n"
            "  ${_sentinel}\n"
            "  The package structure may be incorrect.  Delete '${_pkg_dest}' and retry.")
    endif()

    message(STATUS "alloy-devices: ${vendor}/${family} ready at ${_cache_dir}/${vendor}/${family}")
endfunction()

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
    set(ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_USB_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_IRQ_SEMANTICS_AVAILABLE FALSE PARENT_SCOPE)
    set(ALLOY_DEVICE_INCLUDE_DIRS "" PARENT_SCOPE)
    set(ALLOY_DEVICE_STARTUP_VECTORS_SOURCE "" PARENT_SCOPE)
    set(ALLOY_DEVICE_GENERATED_STARTUP_SOURCE "" PARENT_SCOPE)
    set(ALLOY_DESCRIPTOR_RUNTIME_ENABLED FALSE PARENT_SCOPE)

    # -- Resolve board → vendor/family/device/arch ----------------------------
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

    # -- Resolve _devices_root via Mode B then Mode A -------------------------
    set(_devices_root "")

    # Mode B: explicit or default local checkout
    if(ALLOY_DEVICES_ROOT AND NOT "${ALLOY_DEVICES_ROOT}" STREQUAL "")
        get_filename_component(_explicit_root "${ALLOY_DEVICES_ROOT}" ABSOLUTE)
        if(EXISTS "${_explicit_root}")
            set(_devices_root "${_explicit_root}")
            message(STATUS "alloy-devices: Mode B — local checkout at ${_devices_root}")
        endif()
    endif()

    # Mode A: package cache / auto-download
    if(NOT _devices_root AND ALLOY_USE_ALLOY_DEVICES AND _vendor AND _family)
        _alloy_resolve_device_cache_dir(_cache_dir)
        set(_family_cache_sentinel "${_cache_dir}/${_vendor}/${_family}/generated/runtime/types.hpp")

        if(NOT EXISTS "${_family_cache_sentinel}")
            _alloy_ensure_device_package("${_vendor}" "${_family}")
        endif()

        if(EXISTS "${_family_cache_sentinel}")
            set(_devices_root "${_cache_dir}")
            message(STATUS "alloy-devices: Mode A — package cache at ${_cache_dir}")
        endif()
    endif()

    set(ALLOY_DEVICES_ROOT_ABS "${_devices_root}" PARENT_SCOPE)

    # -------------------------------------------------------------------------
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
    set(_qspi_semantics_available 0)
    set(_sdmmc_semantics_available 0)
    set(_eth_semantics_available 0)
    set(_usb_semantics_available 0)
    set(_irq_semantics_available 0)
    set(_descriptor_runtime_enabled 0)
    set(_family_root "")
    set(_startup_vectors "")
    set(_generated_startup_source "")
    # alloy.device.v2.1 flat-struct codegen format (alloy/device/ tree)
    set(_codegen_format_available 0)
    set(_codegen_device_dir "")

    if(ALLOY_USE_ALLOY_DEVICES AND _vendor AND _devices_root)
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

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/qspi.hpp")
            set(_qspi_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/sdmmc.hpp")
            set(_sdmmc_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/eth.hpp")
            set(_eth_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/usb.hpp")
            set(_usb_semantics_available 1)
        endif()

        if(EXISTS "${_family_root}/generated/runtime/devices/${_device}/driver_semantics/irq.hpp")
            set(_irq_semantics_available 1)
        endif()

        if(_runtime_available)
            set(_contract_available 1)
            set(_descriptor_runtime_enabled 1)
        endif()
    endif()

    # -- alloy.device.v2.1 flat-struct codegen format --------------------------
    # Detected when alloy/device/<vendor>/<family>/<device>/peripheral_traits.h
    # and peripheral_id.hpp exist (written by alloy-cli / alloy-codegen).
    if(_vendor AND _family AND _device)
        set(_codegen_device_dir
            "${CMAKE_CURRENT_SOURCE_DIR}/device/${_vendor}/${_family}/${_device}"
        )
        if(
            EXISTS "${_codegen_device_dir}/peripheral_traits.h"
            AND EXISTS "${_codegen_device_dir}/peripheral_id.hpp"
        )
            set(_codegen_format_available 1)
            message(STATUS
                "alloy-codegen: v2.1 artifacts at ${_codegen_device_dir}")
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
    set(ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE_INT "${_qspi_semantics_available}")
    set(ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE_INT "${_sdmmc_semantics_available}")
    set(ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE_INT "${_eth_semantics_available}")
    set(ALLOY_DEVICE_USB_SEMANTICS_AVAILABLE_INT "${_usb_semantics_available}")
    set(ALLOY_DEVICE_IRQ_SEMANTICS_AVAILABLE_INT "${_irq_semantics_available}")
    set(ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE_INT "${_codegen_format_available}")
    set(ALLOY_DEVICE_SELECTED_VENDOR "${_vendor}")
    set(ALLOY_DEVICE_SELECTED_FAMILY "${_family}")
    set(ALLOY_DEVICE_SELECTED_NAME "${_device}")
    set(ALLOY_DEVICE_SELECTED_ARCH "${_arch}")

    configure_file(
        "${ALLOY_DEVICES_MODULE_DIR}/templates/selected_config.hpp.in"
        "${_selected_config_header}"
        @ONLY
    )

    # When codegen format is present, prepend the alloy/device/ tree so that
    # #include "st/stm32g0/stm32g071rb/peripheral_traits.h" works from any TU.
    if(_codegen_format_available)
        set(
            ALLOY_DEVICE_INCLUDE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/device;${_devices_root};${CMAKE_BINARY_DIR}/generated"
            PARENT_SCOPE
        )
    else()
        set(
            ALLOY_DEVICE_INCLUDE_DIRS
            "${_devices_root};${CMAKE_BINARY_DIR}/generated"
            PARENT_SCOPE
        )
    endif()
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
    set(ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE "${_qspi_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE "${_sdmmc_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE "${_eth_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_USB_SEMANTICS_AVAILABLE "${_usb_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_IRQ_SEMANTICS_AVAILABLE "${_irq_semantics_available}" PARENT_SCOPE)
    set(ALLOY_DESCRIPTOR_RUNTIME_ENABLED "${_descriptor_runtime_enabled}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE "${_codegen_format_available}" PARENT_SCOPE)
    set(ALLOY_DEVICE_CODEGEN_DIR "${_codegen_device_dir}" PARENT_SCOPE)

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
