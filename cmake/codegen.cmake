# Alloy Code Generator CMake Module
#
# Locates pre-generated code for the specified MCU.
# Code is generated offline by tools/codegen/generate_all.py
#
# Usage:
#   include(codegen)
#   alloy_use_generated_code(
#       MCU <mcu_name>              # e.g., STM32F103C8
#       [VENDOR <vendor>]           # Optional: e.g., st (auto-detected if not provided)
#       [FAMILY <family>]           # Optional: e.g., stm32f1 (auto-detected if not provided)
#   )
#
# Exports:
#   ALLOY_GENERATED_DIR         - Directory containing generated files
#   ALLOY_GENERATED_SOURCES     - List of generated source files
#   ALLOY_GENERATED_HEADERS     - List of generated header files
#   ALLOY_CODEGEN_AVAILABLE     - TRUE if generated code found
#

# Base directory for generated code
set(ALLOY_GENERATED_BASE "${ALLOY_ROOT}/src/generated" CACHE PATH "Generated code base directory")

#
# Auto-detect vendor from MCU name
#
function(alloy_detect_vendor mcu_name out_vendor)
    string(TOLOWER "${mcu_name}" mcu_lower)

    if(mcu_lower MATCHES "^stm32")
        set(${out_vendor} "st" PARENT_SCOPE)
    elseif(mcu_lower MATCHES "^nrf")
        set(${out_vendor} "nordic" PARENT_SCOPE)
    elseif(mcu_lower MATCHES "^rp2040")
        set(${out_vendor} "raspberrypi" PARENT_SCOPE)
    elseif(mcu_lower MATCHES "^sam")
        set(${out_vendor} "atmel" PARENT_SCOPE)
    else()
        set(${out_vendor} "unknown" PARENT_SCOPE)
    endif()
endfunction()

#
# Auto-detect family from MCU name
#
function(alloy_detect_family mcu_name out_family)
    string(TOLOWER "${mcu_name}" mcu_lower)

    # STM32 families: STM32F103 -> stm32f1
    if(mcu_lower MATCHES "^stm32([a-z])([0-9])")
        set(${out_family} "stm32${CMAKE_MATCH_1}${CMAKE_MATCH_2}" PARENT_SCOPE)
        return()
    endif()

    # nRF families: nRF52840 -> nrf52
    if(mcu_lower MATCHES "^nrf([0-9]+)")
        set(${out_family} "nrf${CMAKE_MATCH_1}" PARENT_SCOPE)
        return()
    endif()

    # Default: use MCU name as family
    set(${out_family} "${mcu_lower}" PARENT_SCOPE)
endfunction()

#
# alloy_use_generated_code()
#
# Main function to locate and use pre-generated code
#
function(alloy_use_generated_code)
    # Parse arguments
    set(options "")
    set(oneValueArgs MCU VENDOR FAMILY)
    set(multiValueArgs "")

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT ARG_MCU)
        message(FATAL_ERROR "alloy_use_generated_code: MCU argument is required")
    endif()

    # Convert MCU name to lowercase for directory lookup
    string(TOLOWER "${ARG_MCU}" mcu_lower)

    # Auto-detect vendor if not provided
    if(NOT ARG_VENDOR)
        alloy_detect_vendor("${ARG_MCU}" detected_vendor)
        set(ARG_VENDOR "${detected_vendor}")
        message(STATUS "Code generation: Auto-detected vendor '${ARG_VENDOR}' for MCU '${ARG_MCU}'")
    endif()

    # Auto-detect family if not provided
    if(NOT ARG_FAMILY)
        alloy_detect_family("${ARG_MCU}" detected_family)
        set(ARG_FAMILY "${detected_family}")
        message(STATUS "Code generation: Auto-detected family '${ARG_FAMILY}' for MCU '${ARG_MCU}'")
    endif()

    # Construct path to generated code
    set(gen_dir "${ALLOY_GENERATED_BASE}/${ARG_VENDOR}/${ARG_FAMILY}/${mcu_lower}")

    # Check if generated code exists
    if(NOT EXISTS "${gen_dir}")
        message(WARNING
            "Generated code not found for MCU '${ARG_MCU}':\n"
            "  Expected: ${gen_dir}\n"
            "  \n"
            "  To generate code, run:\n"
            "    cd tools/codegen\n"
            "    python3 generate_all.py --vendor ${ARG_VENDOR}\n"
        )
        set(ALLOY_CODEGEN_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    # Check for required files
    set(startup_file "${gen_dir}/startup.cpp")
    set(peripherals_file "${gen_dir}/peripherals.hpp")

    if(NOT EXISTS "${startup_file}")
        message(WARNING "startup.cpp not found in: ${gen_dir}")
        set(ALLOY_CODEGEN_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    if(NOT EXISTS "${peripherals_file}")
        message(WARNING "peripherals.hpp not found in: ${gen_dir}")
        set(ALLOY_CODEGEN_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    # Export variables
    set(ALLOY_GENERATED_DIR "${gen_dir}" PARENT_SCOPE)
    set(ALLOY_GENERATED_SOURCES "${startup_file}" PARENT_SCOPE)
    set(ALLOY_GENERATED_HEADERS "${peripherals_file}" PARENT_SCOPE)
    set(ALLOY_CODEGEN_AVAILABLE TRUE PARENT_SCOPE)

    # Export to cache for other CMakeLists.txt files
    set(ALLOY_GENERATED_DIR "${gen_dir}" CACHE PATH "Generated code directory" FORCE)
    set(ALLOY_GENERATED_SOURCES "${startup_file}" CACHE STRING "Generated source files" FORCE)
    set(ALLOY_GENERATED_HEADERS "${peripherals_file}" CACHE STRING "Generated header files" FORCE)

    message(STATUS "Code generation: Using pre-generated code from ${gen_dir}")
    message(STATUS "  Startup: ${startup_file}")
    message(STATUS "  Peripherals: ${peripherals_file}")
endfunction()

#
# Legacy compatibility: alloy_generate_code() calls alloy_use_generated_code()
#
function(alloy_generate_code)
    alloy_use_generated_code(${ARGN})

    # Re-export to parent scope
    set(ALLOY_GENERATED_DIR "${ALLOY_GENERATED_DIR}" PARENT_SCOPE)
    set(ALLOY_GENERATED_SOURCES "${ALLOY_GENERATED_SOURCES}" PARENT_SCOPE)
    set(ALLOY_GENERATED_HEADERS "${ALLOY_GENERATED_HEADERS}" PARENT_SCOPE)
    set(ALLOY_CODEGEN_AVAILABLE "${ALLOY_CODEGEN_AVAILABLE}" PARENT_SCOPE)
endfunction()

message(STATUS "Code generation module loaded (using pre-generated code from src/generated/)")
