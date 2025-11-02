# Alloy Board-Specific Compile Flags
#
# Provides functions to set MCU-specific compiler and linker flags
# for different ARM Cortex-M cores and architectures.

#
# Get CPU-specific flags for ARM Cortex-M
#
function(alloy_get_arm_cpu_flags arch out_cpu_flags out_fpu_flags)
    string(TOLOWER "${arch}" arch_lower)

    # ARM Cortex-M0+
    if(arch_lower MATCHES "cortex-m0plus" OR arch_lower MATCHES "cortex-m0\\+")
        set(cpu_flags "-mcpu=cortex-m0plus;-mthumb")
        set(fpu_flags "")

    # ARM Cortex-M3
    elseif(arch_lower MATCHES "cortex-m3")
        set(cpu_flags "-mcpu=cortex-m3;-mthumb")
        set(fpu_flags "")

    # ARM Cortex-M4 (no FPU)
    elseif(arch_lower MATCHES "cortex-m4" AND NOT arch_lower MATCHES "m4f")
        set(cpu_flags "-mcpu=cortex-m4;-mthumb")
        set(fpu_flags "")

    # ARM Cortex-M4F (with FPU)
    elseif(arch_lower MATCHES "cortex-m4f" OR arch_lower MATCHES "m4.*fpu")
        set(cpu_flags "-mcpu=cortex-m4;-mthumb")
        set(fpu_flags "-mfpu=fpv4-sp-d16;-mfloat-abi=hard")

    # ARM Cortex-M7 (with FPU)
    elseif(arch_lower MATCHES "cortex-m7")
        set(cpu_flags "-mcpu=cortex-m7;-mthumb")
        set(fpu_flags "-mfpu=fpv5-d16;-mfloat-abi=hard")

    # Default to Cortex-M3
    else()
        message(WARNING "Unknown ARM architecture '${arch}', defaulting to Cortex-M3")
        set(cpu_flags "-mcpu=cortex-m3;-mthumb")
        set(fpu_flags "")
    endif()

    set(${out_cpu_flags} "${cpu_flags}" PARENT_SCOPE)
    set(${out_fpu_flags} "${fpu_flags}" PARENT_SCOPE)
endfunction()

#
# Apply board-specific compile flags to a target
#
# Usage:
#   alloy_target_board_flags(my_target)
#
function(alloy_target_board_flags target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Get board architecture
    if(NOT DEFINED ALLOY_ARCH)
        message(WARNING "ALLOY_ARCH not defined, skipping board-specific flags")
        return()
    endif()

    # ARM Cortex-M architectures
    if(ALLOY_ARCH MATCHES "arm-cortex")
        alloy_get_arm_cpu_flags("${ALLOY_ARCH}" cpu_flags fpu_flags)

        target_compile_options(${target_name} PRIVATE
            ${cpu_flags}
            ${fpu_flags}
        )

        target_link_options(${target_name} PRIVATE
            ${cpu_flags}
            ${fpu_flags}
        )

        message(STATUS "Applied ARM flags to ${target_name}: ${cpu_flags} ${fpu_flags}")

    # Xtensa architectures
    elseif(ALLOY_ARCH MATCHES "xtensa")
        # Xtensa flags are already set in toolchain file
        message(STATUS "Xtensa architecture detected for ${target_name}")

    # RISC-V architectures (future support)
    elseif(ALLOY_ARCH MATCHES "riscv")
        message(STATUS "RISC-V architecture detected for ${target_name} (not yet implemented)")

    # Host architecture
    elseif(ALLOY_ARCH MATCHES "host" OR NOT DEFINED ALLOY_ARCH)
        message(STATUS "Host architecture for ${target_name}, using native flags")

    else()
        message(WARNING "Unknown architecture '${ALLOY_ARCH}' for ${target_name}")
    endif()
endfunction()

#
# Create flags for a specific board configuration
#
# Returns a string of compile flags suitable for CMAKE_C_FLAGS/CMAKE_CXX_FLAGS
#
function(alloy_get_board_compile_flags out_flags)
    set(flags "")

    if(NOT DEFINED ALLOY_ARCH)
        set(${out_flags} "" PARENT_SCOPE)
        return()
    endif()

    # ARM Cortex-M architectures
    if(ALLOY_ARCH MATCHES "arm-cortex")
        alloy_get_arm_cpu_flags("${ALLOY_ARCH}" cpu_flags fpu_flags)
        set(flags "${cpu_flags} ${fpu_flags}")
    endif()

    set(${out_flags} "${flags}" PARENT_SCOPE)
endfunction()

#
# Get linker script for current board
#
function(alloy_get_linker_script out_linker_script)
    if(NOT DEFINED ALLOY_MCU)
        set(${out_linker_script} "" PARENT_SCOPE)
        return()
    endif()

    # Board-specific linker scripts
    set(boards_dir "${ALLOY_ROOT}/boards")

    # Map board names to linker script locations
    if(ALLOY_BOARD STREQUAL "bluepill")
        set(linker_script "${boards_dir}/stm32f103c8/STM32F103C8.ld")
    elseif(ALLOY_BOARD STREQUAL "esp32_devkit")
        set(linker_script "${boards_dir}/esp32_devkit/esp32.ld")
    elseif(ALLOY_BOARD STREQUAL "stm32f407vg")
        set(linker_script "${boards_dir}/stm32f407vg/STM32F407VG.ld")
    elseif(ALLOY_BOARD STREQUAL "arduino_zero")
        set(linker_script "${boards_dir}/arduino_zero/ATSAMD21G18.ld")
    elseif(ALLOY_BOARD STREQUAL "rp_pico")
        set(linker_script "${boards_dir}/raspberry_pi_pico/RP2040.ld")
    elseif(ALLOY_BOARD STREQUAL "rp2040_zero")
        set(linker_script "${boards_dir}/waveshare_rp2040_zero/RP2040.ld")
    else()
        set(linker_script "")
    endif()

    if(linker_script AND EXISTS "${linker_script}")
        set(${out_linker_script} "${linker_script}" PARENT_SCOPE)
    else()
        set(${out_linker_script} "" PARENT_SCOPE)
    endif()
endfunction()

#
# Apply linker script to target
#
function(alloy_target_linker_script target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    alloy_get_linker_script(linker_script)

    if(linker_script)
        target_link_options(${target_name} PRIVATE
            -T${linker_script}
        )
        message(STATUS "Applied linker script to ${target_name}: ${linker_script}")
    else()
        message(WARNING "No linker script found for board '${ALLOY_BOARD}'")
    endif()
endfunction()

#
# Link required libraries for bare-metal ARM targets
#
# For xPack ARM toolchain v14+, we can't use spec files due to conflicts,
# so we link libraries explicitly in the correct order.
#
function(alloy_target_arm_libraries target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist")
    endif()

    # Only apply to ARM targets
    if(ALLOY_ARCH MATCHES "arm")
        # Disable default libraries and link explicitly to avoid conflicts
        # -nodefaultlibs: Don't link standard system libraries automatically
        # Then we manually link only what we need in the right order
        target_link_options(${target_name} PRIVATE
            -nodefaultlibs
            -Wl,--start-group
            -lc_nano
            -lnosys
            -lgcc
            -lm
            -lstdc++_nano
            -lsupc++
            -Wl,--end-group
        )
        message(STATUS "Linked ARM bare-metal libraries to ${target_name}")
    endif()
endfunction()

#
# Convenience function to apply all board-specific settings to a target
#
# Usage:
#   alloy_target_board_settings(my_firmware)
#
function(alloy_target_board_settings target_name)
    alloy_target_board_flags(${target_name})
    alloy_target_linker_script(${target_name})
    alloy_target_arm_libraries(${target_name})
endfunction()
