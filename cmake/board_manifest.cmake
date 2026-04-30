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

    # -- JSON auto-discovery (board-manifest-declarative spec) ----------------
    # Try boards/<board_name>/board.json first (O(1) direct path lookup).
    # Falls back to GLOB_RECURSE for boards in ALLOY_CUSTOM_BOARD_DIR or nested
    # subdirectories.  The legacy if/elseif chain below remains as a fallback.
    if(NOT BOARD_NAME STREQUAL "custom" AND NOT BOARD_NAME STREQUAL "host")
        # -- Stamp file fast-path (task 3.4) --------------------------------
        # For boards whose board.json is at the canonical direct path, cache
        # the resolved variables in a stamp file under CMAKE_BINARY_DIR.
        # The stamp is invalidated automatically when board.json changes via
        # CMAKE_CONFIGURE_DEPENDS.
        set(_alloy_stamp_dir "${CMAKE_BINARY_DIR}/_alloy_board_stamps")
        set(_alloy_stamp_file "${_alloy_stamp_dir}/${BOARD_NAME}.cmake")
        set(_alloy_stamp_hit FALSE)
        set(_alloy_json_for_stamp "")

        set(_direct_json_fast "${CMAKE_SOURCE_DIR}/boards/${BOARD_NAME}/board.json")
        if(EXISTS "${_direct_json_fast}")
            # Register configure dependency so cmake re-runs when board.json changes
            set_property(DIRECTORY PROPERTY
                CMAKE_CONFIGURE_DEPENDS "${_direct_json_fast}")
            set(_alloy_json_for_stamp "${_direct_json_fast}")

            if(EXISTS "${_alloy_stamp_file}")
                set(_ALLOY_STAMP_HASH "")
                include("${_alloy_stamp_file}")
                file(SHA256 "${_direct_json_fast}" _alloy_cur_hash)
                if("${_ALLOY_STAMP_HASH}" STREQUAL "${_alloy_cur_hash}"
                   AND NOT "${_ALLOY_STAMP_HASH}" STREQUAL "")
                    set(_alloy_stamp_hit TRUE)
                    set(_found    TRUE)
                    set(_vendor   "${_ALLOY_STAMP_VENDOR}")
                    set(_family   "${_ALLOY_STAMP_FAMILY}")
                    set(_device   "${_ALLOY_STAMP_DEVICE}")
                    set(_arch     "${_ALLOY_STAMP_ARCH}")
                    set(_board_header   "${_ALLOY_STAMP_BOARD_HEADER}")
                    set(_linker_script  "${_ALLOY_STAMP_LINKER_SCRIPT}")
                    set(_mcu            "${_ALLOY_STAMP_MCU}")
                    set(_flash_size_bytes             "${_ALLOY_STAMP_FLASH_SIZE_BYTES}")
                    set(_supports_uart_logger         "${_ALLOY_STAMP_UART_LOGGER}")
                    set(_supports_dma_probe           "${_ALLOY_STAMP_DMA_PROBE}")
                    set(_supports_peripheral_examples "${_ALLOY_STAMP_PERIPHERAL_EXAMPLES}")
                endif()
            endif()
        endif()

        if(_alloy_stamp_hit)
            # Skip JSON parsing — use cached values from stamp
        else()
        # -- End stamp file fast-path ----------------------------------------

        set(_json_candidates "")

        # 1. Direct subdirectory: boards/<name>/board.json
        set(_direct_json "${CMAKE_SOURCE_DIR}/boards/${BOARD_NAME}/board.json")
        if(EXISTS "${_direct_json}")
            list(APPEND _json_candidates "${_direct_json}")
        endif()

        # 2. Custom board directory
        if(DEFINED ALLOY_CUSTOM_BOARD_DIR AND NOT "${ALLOY_CUSTOM_BOARD_DIR}" STREQUAL "")
            set(_custom_json "${ALLOY_CUSTOM_BOARD_DIR}/board.json")
            if(EXISTS "${_custom_json}")
                list(APPEND _json_candidates "${_custom_json}")
            endif()
        endif()

        # 3. Glob for nested boards (only when direct lookup failed)
        if(NOT _json_candidates)
            file(GLOB_RECURSE _all_manifests
                "${CMAKE_SOURCE_DIR}/boards/*/board.json")
            foreach(_m ${_all_manifests})
                file(READ "${_m}" _mj)
                string(JSON _mid ERROR_VARIABLE _je GET "${_mj}" "board_id")
                if(_je STREQUAL "" AND "${_mid}" STREQUAL "${BOARD_NAME}")
                    list(APPEND _json_candidates "${_m}")
                endif()
            endforeach()
        endif()

        # Parse the first matching manifest
        foreach(_json_path ${_json_candidates})
            file(READ "${_json_path}" _json_content)
            string(JSON _json_id   ERROR_VARIABLE _je GET "${_json_content}" "board_id")
            if(NOT _je STREQUAL "")
                continue()
            endif()
            if(NOT "${_json_id}" STREQUAL "${BOARD_NAME}" AND NOT "${BOARD_NAME}" STREQUAL "same70_xpld")
                # Allow same70_xpld → same70_xplained alias (legacy)
                if(NOT ("${BOARD_NAME}" STREQUAL "same70_xpld" AND "${_json_id}" STREQUAL "same70_xplained"))
                    continue()
                endif()
            endif()

            # -- Schema version check (task 3.3) ----------------------------------
            # Reader supports: v1 (major=1).  Warn on unknown; fail on major > 1.
            set(_ALLOY_BOARD_MANIFEST_READER_MAJOR 1)
            string(JSON _j_schema ERROR_VARIABLE _je GET "${_json_content}" "\$schema")
            if(NOT _je STREQUAL "" OR "${_j_schema}" STREQUAL "")
                # No $schema field — silently accept (pre-schema boards)
            else()
                string(REGEX MATCH "/v([0-9]+)\\.json$" _schema_match "${_j_schema}")
                if(_schema_match)
                    string(REGEX REPLACE "/v([0-9]+)\\.json$" "\\1" _schema_major "${_j_schema}")
                    if(_schema_major GREATER _ALLOY_BOARD_MANIFEST_READER_MAJOR)
                        message(FATAL_ERROR
                            "alloy board-manifest: ${_json_path}\n"
                            "  Schema major version ${_schema_major} is newer than this CMake reader "
                            "(supports up to v${_ALLOY_BOARD_MANIFEST_READER_MAJOR}).\n"
                            "  Update alloy to a version that understands board-manifest v${_schema_major}.")
                    elseif(_schema_major LESS _ALLOY_BOARD_MANIFEST_READER_MAJOR)
                        message(WARNING
                            "alloy board-manifest: ${_json_path}: "
                            "schema v${_schema_major} is older than reader "
                            "(v${_ALLOY_BOARD_MANIFEST_READER_MAJOR}) — consider upgrading the board.json.")
                    endif()
                endif()
            endif()
            # -- End schema version check -----------------------------------------

            # Extract required fields
            string(JSON _j_vendor   ERROR_VARIABLE _je GET "${_json_content}" "vendor")
            string(JSON _j_family   ERROR_VARIABLE _je GET "${_json_content}" "family")
            string(JSON _j_device   ERROR_VARIABLE _je GET "${_json_content}" "device")
            string(JSON _j_arch     ERROR_VARIABLE _je GET "${_json_content}" "arch")
            string(JSON _j_linker   ERROR_VARIABLE _je GET "${_json_content}" "linker_script")
            string(JSON _j_header   ERROR_VARIABLE _je GET "${_json_content}" "board_header")
            # Extract optional fields
            string(JSON _j_mcu      ERROR_VARIABLE _je GET "${_json_content}" "mcu")
            string(JSON _j_flash    ERROR_VARIABLE _je GET "${_json_content}" "flash_size_bytes")

            if(_j_vendor AND _j_family AND _j_device AND _j_arch AND _j_linker AND _j_header)
                set(_found TRUE)
                set(_vendor "${_j_vendor}")
                set(_family "${_j_family}")
                set(_device "${_j_device}")
                set(_arch   "${_j_arch}")
                set(_board_header "${_j_header}")

                # Linker script: resolve relative to the board.json directory
                get_filename_component(_json_dir "${_json_path}" DIRECTORY)
                if(IS_ABSOLUTE "${_j_linker}")
                    set(_linker_script "${_j_linker}")
                else()
                    set(_linker_script "${_json_dir}/${_j_linker}")
                endif()

                if(_j_mcu)
                    set(_mcu "${_j_mcu}")
                endif()
                if(_j_flash AND NOT "${_j_flash}" STREQUAL "")
                    set(_flash_size_bytes "${_j_flash}")
                endif()

                # Infer legacy support flags from firmware_targets list
                string(JSON _j_targets_len ERROR_VARIABLE _je
                    LENGTH "${_json_content}" "firmware_targets")
                if(_je STREQUAL "" AND _j_targets_len GREATER 0)
                    set(_supports_uart_logger FALSE)
                    set(_supports_dma_probe FALSE)
                    set(_supports_peripheral_examples FALSE)
                    foreach(_ti RANGE 0 ${_j_targets_len})
                        math(EXPR _ti_end "${_j_targets_len} - 1")
                        if(_ti GREATER _ti_end)
                            break()
                        endif()
                        string(JSON _t ERROR_VARIABLE _je
                            GET "${_json_content}" "firmware_targets" "${_ti}")
                        if(_t STREQUAL "uart_logger")
                            set(_supports_uart_logger TRUE)
                        endif()
                        if(_t STREQUAL "dma_probe")
                            set(_supports_dma_probe TRUE)
                        endif()
                        if(_t STREQUAL "i2c_scan" OR _t STREQUAL "spi_probe"
                           OR _t STREQUAL "analog_probe" OR _t STREQUAL "can_probe")
                            set(_supports_peripheral_examples TRUE)
                        endif()
                    endforeach()
                endif()

                message(STATUS
                    "alloy board-manifest: '${BOARD_NAME}' resolved from ${_json_path}")

                # Write stamp for next configure
                if(_alloy_json_for_stamp AND "${_json_path}" STREQUAL "${_alloy_json_for_stamp}")
                    file(SHA256 "${_json_path}" _alloy_new_hash)
                    file(MAKE_DIRECTORY "${_alloy_stamp_dir}")
                    file(WRITE "${_alloy_stamp_file}"
"# alloy board-manifest stamp — auto-generated. Do not edit.
# Invalidated automatically when board.json changes (CMAKE_CONFIGURE_DEPENDS).
set(_ALLOY_STAMP_HASH \"${_alloy_new_hash}\")
set(_ALLOY_STAMP_VENDOR \"${_vendor}\")
set(_ALLOY_STAMP_FAMILY \"${_family}\")
set(_ALLOY_STAMP_DEVICE \"${_device}\")
set(_ALLOY_STAMP_ARCH \"${_arch}\")
set(_ALLOY_STAMP_BOARD_HEADER \"${_board_header}\")
set(_ALLOY_STAMP_LINKER_SCRIPT \"${_linker_script}\")
set(_ALLOY_STAMP_MCU \"${_mcu}\")
set(_ALLOY_STAMP_FLASH_SIZE_BYTES \"${_flash_size_bytes}\")
set(_ALLOY_STAMP_UART_LOGGER \"${_supports_uart_logger}\")
set(_ALLOY_STAMP_DMA_PROBE \"${_supports_dma_probe}\")
set(_ALLOY_STAMP_PERIPHERAL_EXAMPLES \"${_supports_peripheral_examples}\")
")
                endif()

                break()
            endif()
        endforeach()
        endif()  # end else (stamp miss branch)
    endif()
    # -- End JSON auto-discovery ----------------------------------------------

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
    elseif(NOT _found AND BOARD_NAME STREQUAL "host")
        set(_found TRUE)
        set(_board_header "boards/linux_host/board.hpp")
        set(_arch "native")
        set(_mcu "native")
        set(_flash_size_bytes 0)
    elseif(NOT _found AND BOARD_NAME STREQUAL "nucleo_g071rb")
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
        set(_supports_peripheral_examples TRUE)
    elseif(NOT _found AND BOARD_NAME STREQUAL "nucleo_g0b1re")
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
    elseif(NOT _found AND BOARD_NAME STREQUAL "nucleo_f401re")
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
        set(_supports_peripheral_examples TRUE)
    elseif(NOT _found AND (BOARD_NAME STREQUAL "same70_xpld" OR BOARD_NAME STREQUAL "same70_xplained"))
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
    elseif(NOT _found AND BOARD_NAME STREQUAL "raspberry_pi_pico")
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
    elseif(NOT _found AND BOARD_NAME STREQUAL "avr128da32_curiosity_nano")
        set(_found TRUE)
        set(_board_header "boards/avr128da32_curiosity_nano/board.hpp")
        set(_vendor "microchip")
        set(_family "avr-da")
        set(_device "avr128da32")
        set(_arch "avr")
        set(_mcu "AVR128DA32")
        set(_flash_size_bytes 131072)
        set(_supports_uart_logger TRUE)
    elseif(NOT _found AND BOARD_NAME STREQUAL "esp32c3_devkitm")
        set(_found TRUE)
        set(_board_header "boards/esp32c3_devkitm/board.hpp")
        set(_vendor "espressif")
        set(_family "esp32c3")
        set(_device "esp32c3")
        set(_arch "riscv32")
        set(_mcu "ESP32-C3")
        set(_flash_size_bytes 4194304)
        set(_supports_uart_logger TRUE)
    elseif(NOT _found AND BOARD_NAME STREQUAL "esp32s3_devkitc")
        set(_found TRUE)
        set(_board_header "boards/esp32s3_devkitc/board.hpp")
        set(_vendor "espressif")
        set(_family "esp32s3")
        set(_device "esp32s3")
        set(_arch "xtensa-lx7")
        set(_mcu "ESP32-S3")
        set(_flash_size_bytes 8388608)
        set(_supports_uart_logger TRUE)
    elseif(NOT _found AND BOARD_NAME STREQUAL "esp32_devkit")
        set(_found TRUE)
        set(_board_header "boards/esp32_devkit/board.hpp")
        set(_vendor "espressif")
        set(_family "esp32")
        set(_device "esp32")
        set(_arch "xtensa-lx6")
        set(_mcu "ESP32")
        set(_flash_size_bytes 4194304)
        set(_supports_uart_logger TRUE)
    elseif(NOT _found AND BOARD_NAME STREQUAL "esp_wrover_kit")
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
