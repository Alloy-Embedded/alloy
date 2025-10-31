# Alloy Code Generator CMake Module
#
# Provides automatic code generation from MCU database JSON files.
#
# Usage:
#   include(codegen)
#   alloy_generate_code(
#       MCU <mcu_name>              # e.g., STM32F103C8
#       [DATABASE <path>]           # Optional: specific database file
#       [OUTPUT_DIR <path>]         # Optional: output directory (default: ${CMAKE_BINARY_DIR}/generated)
#       [FAMILY <family_name>]      # Optional: family name for auto-detection
#   )
#
# Exports:
#   ALLOY_GENERATED_DIR         - Directory containing generated files
#   ALLOY_GENERATED_SOURCES     - List of generated source files
#   ALLOY_GENERATED_HEADERS     - List of generated header files
#

# Find Python3 (required for code generation)
find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_FOUND)
    message(WARNING "Python3 not found - code generation will be disabled")
    set(ALLOY_CODEGEN_AVAILABLE FALSE CACHE BOOL "Code generation available" FORCE)
    return()
else()
    set(ALLOY_CODEGEN_AVAILABLE TRUE CACHE BOOL "Code generation available" FORCE)
    message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
endif()

# Paths
set(ALLOY_CODEGEN_DIR "${ALLOY_ROOT}/tools/codegen" CACHE PATH "Code generator directory")
set(ALLOY_CODEGEN_SCRIPT "${ALLOY_CODEGEN_DIR}/generator.py" CACHE PATH "Generator script")
set(ALLOY_DATABASE_DIR "${ALLOY_CODEGEN_DIR}/database/families" CACHE PATH "Database directory")

# Check if generator script exists
if(NOT EXISTS "${ALLOY_CODEGEN_SCRIPT}")
    message(WARNING "Code generator not found at: ${ALLOY_CODEGEN_SCRIPT}")
    set(ALLOY_CODEGEN_AVAILABLE FALSE CACHE BOOL "Code generation available" FORCE)
    return()
endif()

#
# alloy_generate_code()
#
# Main function to generate code from MCU database
#
function(alloy_generate_code)
    # Parse arguments
    set(options "")
    set(oneValueArgs MCU DATABASE OUTPUT_DIR FAMILY)
    set(multiValueArgs "")
    cmake_parse_arguments(GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT GEN_MCU)
        message(FATAL_ERROR "alloy_generate_code: MCU argument is required")
    endif()

    # Set default output directory
    if(NOT GEN_OUTPUT_DIR)
        set(GEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/${GEN_MCU}")
    endif()

    # Auto-detect database file if not specified
    if(NOT GEN_DATABASE)
        # Try to find database file automatically
        if(GEN_FAMILY)
            # Use specified family name
            set(DATABASE_FILE "${ALLOY_DATABASE_DIR}/${GEN_FAMILY}.json")
        else()
            # Try to infer family from MCU name
            # Common patterns: STM32F103C8 -> stm32f1xx, NRF52840 -> nrf52
            string(TOLOWER "${GEN_MCU}" mcu_lower)

            # STM32 pattern
            if(mcu_lower MATCHES "^stm32([a-z][0-9])")
                set(family_guess "stm32${CMAKE_MATCH_1}xx")
                set(DATABASE_FILE "${ALLOY_DATABASE_DIR}/${family_guess}.json")
            # nRF pattern
            elseif(mcu_lower MATCHES "^nrf([0-9]+)")
                set(family_guess "nrf${CMAKE_MATCH_1}")
                set(DATABASE_FILE "${ALLOY_DATABASE_DIR}/${family_guess}.json")
            # RL78 pattern
            elseif(mcu_lower MATCHES "^rl78")
                set(DATABASE_FILE "${ALLOY_DATABASE_DIR}/rl78.json")
            # ESP32 pattern
            elseif(mcu_lower MATCHES "^esp32")
                set(DATABASE_FILE "${ALLOY_DATABASE_DIR}/esp32.json")
            else()
                message(FATAL_ERROR
                    "alloy_generate_code: Cannot auto-detect database for MCU '${GEN_MCU}'. "
                    "Please specify DATABASE or FAMILY argument.")
            endif()
        endif()

        set(GEN_DATABASE "${DATABASE_FILE}")
    endif()

    # Validate database file exists
    if(NOT EXISTS "${GEN_DATABASE}")
        message(FATAL_ERROR
            "alloy_generate_code: Database file not found: ${GEN_DATABASE}\n"
            "  MCU: ${GEN_MCU}\n"
            "  Available databases in ${ALLOY_DATABASE_DIR}:\n"
            "    - Run 'ls ${ALLOY_DATABASE_DIR}' to see available families")
    endif()

    # Marker file for tracking generation
    set(MARKER_FILE "${GEN_OUTPUT_DIR}/.generated")

    # Determine if we need to regenerate
    set(NEED_GENERATE FALSE)

    if(NOT EXISTS "${MARKER_FILE}")
        set(NEED_GENERATE TRUE)
        set(GENERATION_REASON "marker file missing")
    else()
        # Check if database is newer than marker
        file(TIMESTAMP "${GEN_DATABASE}" DB_TIME)
        file(TIMESTAMP "${MARKER_FILE}" MARKER_TIME)

        if(DB_TIME IS_NEWER_THAN MARKER_TIME)
            set(NEED_GENERATE TRUE)
            set(GENERATION_REASON "database updated")
        endif()

        # Check if generator script is newer
        file(TIMESTAMP "${ALLOY_CODEGEN_SCRIPT}" SCRIPT_TIME)
        if(SCRIPT_TIME IS_NEWER_THAN MARKER_TIME)
            set(NEED_GENERATE TRUE)
            set(GENERATION_REASON "generator updated")
        endif()
    endif()

    # Generate code if needed
    if(NEED_GENERATE)
        message(STATUS "Generating code for ${GEN_MCU} (${GENERATION_REASON})...")

        # Ensure output directory exists
        file(MAKE_DIRECTORY "${GEN_OUTPUT_DIR}")

        # Run generator
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${ALLOY_CODEGEN_SCRIPT}"
                --mcu "${GEN_MCU}"
                --database "${GEN_DATABASE}"
                --output "${GEN_OUTPUT_DIR}"
                --verbose
            WORKING_DIRECTORY "${ALLOY_CODEGEN_DIR}"
            RESULT_VARIABLE GEN_RESULT
            OUTPUT_VARIABLE GEN_OUTPUT
            ERROR_VARIABLE GEN_ERROR
        )

        # Check result
        if(NOT GEN_RESULT EQUAL 0)
            message(FATAL_ERROR
                "Code generation failed for ${GEN_MCU}!\n"
                "Command: ${Python3_EXECUTABLE} ${ALLOY_CODEGEN_SCRIPT}\n"
                "Database: ${GEN_DATABASE}\n"
                "Output: ${GEN_OUTPUT}\n"
                "Error: ${GEN_ERROR}")
        endif()

        # Create marker file
        file(WRITE "${MARKER_FILE}"
            "Generated: ${CMAKE_CURRENT_LIST_FILE}\n"
            "MCU: ${GEN_MCU}\n"
            "Database: ${GEN_DATABASE}\n"
            "Timestamp: ${DB_TIME}\n")

        message(STATUS "Code generation complete: ${GEN_OUTPUT_DIR}")
    else()
        message(STATUS "Using cached generated code for ${GEN_MCU}")
    endif()

    # Export variables to parent scope
    set(ALLOY_GENERATED_DIR "${GEN_OUTPUT_DIR}" PARENT_SCOPE)

    # Collect generated source files
    file(GLOB GENERATED_SOURCES
        "${GEN_OUTPUT_DIR}/*.cpp"
        "${GEN_OUTPUT_DIR}/*.c")
    set(ALLOY_GENERATED_SOURCES "${GENERATED_SOURCES}" PARENT_SCOPE)

    # Collect generated header files
    file(GLOB GENERATED_HEADERS
        "${GEN_OUTPUT_DIR}/*.h"
        "${GEN_OUTPUT_DIR}/*.hpp")
    set(ALLOY_GENERATED_HEADERS "${GENERATED_HEADERS}" PARENT_SCOPE)

    # Display generated files
    if(GENERATED_SOURCES)
        message(STATUS "Generated sources:")
        foreach(src ${GENERATED_SOURCES})
            message(STATUS "  - ${src}")
        endforeach()
    endif()

    if(GENERATED_HEADERS)
        message(STATUS "Generated headers:")
        foreach(hdr ${GENERATED_HEADERS})
            message(STATUS "  - ${hdr}")
        endforeach()
    endif()

endfunction()

#
# alloy_validate_database()
#
# Validate a database JSON file
#
function(alloy_validate_database DATABASE_FILE)
    if(NOT ALLOY_CODEGEN_AVAILABLE)
        message(WARNING "Cannot validate database - Python3 not available")
        return()
    endif()

    set(VALIDATOR_SCRIPT "${ALLOY_CODEGEN_DIR}/validate_database.py")

    if(NOT EXISTS "${VALIDATOR_SCRIPT}")
        message(WARNING "Database validator not found: ${VALIDATOR_SCRIPT}")
        return()
    endif()

    message(STATUS "Validating database: ${DATABASE_FILE}")

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${VALIDATOR_SCRIPT}" "${DATABASE_FILE}"
        WORKING_DIRECTORY "${ALLOY_CODEGEN_DIR}"
        RESULT_VARIABLE VAL_RESULT
        OUTPUT_VARIABLE VAL_OUTPUT
        ERROR_VARIABLE VAL_ERROR
    )

    if(NOT VAL_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Database validation failed!\n"
            "File: ${DATABASE_FILE}\n"
            "Output: ${VAL_OUTPUT}\n"
            "Error: ${VAL_ERROR}")
    else()
        message(STATUS "Database validation passed")
    endif()
endfunction()

# Display code generation status
if(ALLOY_CODEGEN_AVAILABLE)
    message(STATUS "Code generation: ENABLED")
    message(STATUS "  Generator: ${ALLOY_CODEGEN_SCRIPT}")
    message(STATUS "  Databases: ${ALLOY_DATABASE_DIR}")
else()
    message(STATUS "Code generation: DISABLED (Python3 not found)")
endif()
