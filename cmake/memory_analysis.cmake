# Memory Analysis and Optimization Module
#
# Provides memory footprint analysis and size optimization for low-memory MCUs.
#
# Functions:
# - alloy_add_memory_analysis(target) - Add memory reporting to a target
# - alloy_apply_minimal_flags(target) - Apply size optimization flags
#
# Variables set by this module:
# - ALLOY_RAM_SIZE - Target RAM size in bytes (set by board config)
# - ALLOY_FLASH_SIZE - Target Flash size in bytes (set by board config)

# Apply minimal build flags for size optimization
#
# Usage:
#   alloy_apply_minimal_flags(my_target)
#
# Flags applied:
# - Compilation: -Os (optimize for size), -flto (link-time optimization)
# - Linker: -Wl,--gc-sections (remove unused sections), -flto
#
function(alloy_apply_minimal_flags TARGET_NAME)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "alloy_apply_minimal_flags: Target '${TARGET_NAME}' does not exist")
    endif()

    message(STATUS "Applying minimal build flags to ${TARGET_NAME}")

    # Compilation flags for size optimization
    target_compile_options(${TARGET_NAME} PRIVATE
        -Os                      # Optimize for size
        -flto                    # Link-time optimization
        -ffunction-sections      # Place each function in separate section
        -fdata-sections          # Place each data item in separate section
        -fno-exceptions          # Disable C++ exceptions (saves significant space)
        -fno-rtti                # Disable C++ RTTI (saves vtable overhead)
    )

    # Linker flags for dead code elimination
    target_link_options(${TARGET_NAME} PRIVATE
        -Wl,--gc-sections        # Remove unused sections
        -flto                    # Link-time optimization
        -Wl,--print-memory-usage # Print memory usage summary
    )

    message(STATUS "  -Os -flto -ffunction-sections -fdata-sections")
    message(STATUS "  -Wl,--gc-sections -flto")
endfunction()

# Add memory analysis reporting to a target
#
# Usage:
#   alloy_add_memory_analysis(my_target)
#
# Creates custom targets:
# - memory-report-<target> - Generate memory report for specific target
# - memory-report (global) - Generate reports for all targets
#
# Requires:
# - Python 3 (for analyze_memory.py)
# - tools/analyze_memory.py script
#
function(alloy_add_memory_analysis TARGET_NAME)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "alloy_add_memory_analysis: Target '${TARGET_NAME}' does not exist")
    endif()

    # Find Python 3
    find_package(Python3 COMPONENTS Interpreter)
    if(NOT Python3_FOUND)
        message(WARNING "Python 3 not found, memory analysis will not be available")
        return()
    endif()

    # Path to memory analyzer script
    set(ANALYZER_SCRIPT "${CMAKE_SOURCE_DIR}/tools/analyze_memory.py")
    if(NOT EXISTS ${ANALYZER_SCRIPT})
        message(WARNING "Memory analyzer script not found at ${ANALYZER_SCRIPT}")
        return()
    endif()

    # Get target binary path
    set(TARGET_BINARY "$<TARGET_FILE:${TARGET_NAME}>")
    set(MAP_FILE "$<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}.map")

    # Ensure map file is generated
    target_link_options(${TARGET_NAME} PRIVATE
        -Wl,-Map=${MAP_FILE}
    )

    # Create memory report target for this specific target
    add_custom_target(memory-report-${TARGET_NAME}
        COMMAND ${Python3_EXECUTABLE} ${ANALYZER_SCRIPT}
                --elf ${TARGET_BINARY}
                --map ${MAP_FILE}
                --mcu "${ALLOY_MCU}"
                --ram-size ${ALLOY_RAM_SIZE}
                --flash-size ${ALLOY_FLASH_SIZE}
        DEPENDS ${TARGET_NAME}
        COMMENT "Generating memory report for ${TARGET_NAME}"
        VERBATIM
    )

    # Add to global memory-report target (if not already exists)
    if(NOT TARGET memory-report)
        add_custom_target(memory-report
            COMMENT "Generating memory reports for all targets"
        )
    endif()

    # Make global target depend on this target's report
    add_dependencies(memory-report memory-report-${TARGET_NAME})

    message(STATUS "Memory analysis enabled for ${TARGET_NAME}")
    message(STATUS "  Run: cmake --build . --target memory-report-${TARGET_NAME}")
endfunction()

# Set memory size defaults (can be overridden by board configs)
if(NOT ALLOY_RAM_SIZE)
    set(ALLOY_RAM_SIZE 0 CACHE STRING "Target RAM size in bytes")
endif()

if(NOT ALLOY_FLASH_SIZE)
    set(ALLOY_FLASH_SIZE 0 CACHE STRING "Target Flash size in bytes")
endif()

message(STATUS "Memory analysis module loaded")
