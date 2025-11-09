cmake_minimum_required(VERSION 3.25)

# Project name
project(same70_clock_test
    VERSION 1.0.0
    DESCRIPTION "Clock configuration test for SAME70 Xplained Ultra"
    LANGUAGES CXX C ASM
)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
    main.cpp
)

# Add startup code for SAME70Q21 (only for ARM cross-compilation)
if(CMAKE_CROSSCOMPILING AND ALLOY_BOARD MATCHES "same70")
    list(APPEND SOURCES
        ${CMAKE_SOURCE_DIR}/boards/atmel_same70_xpld/startup.cpp
    )
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Link with HAL library
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${ALLOY_HAL_LIBRARY}
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O2 -g>
    $<$<CONFIG:MinSizeRel>:-Os -g>
)

# Embedded-specific flags (only for ARM targets)
if(CMAKE_CROSSCOMPILING)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
    )

    # Add linker script if specified
    if(DEFINED LINKER_SCRIPT AND EXISTS "${LINKER_SCRIPT}")
        target_link_options(${PROJECT_NAME} PRIVATE
            -T${LINKER_SCRIPT}
        )
    endif()

    # Generate additional output formats
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HEX and BIN files for ${PROJECT_NAME}"
    )

    message(STATUS "Building ${PROJECT_NAME} for ${ALLOY_BOARD}")
    message(STATUS "  Output: ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf")
    message(STATUS "          ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex")
    message(STATUS "          ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin")
endif()

# Flash target (requires OpenOCD)
if(CMAKE_CROSSCOMPILING AND ALLOY_BOARD MATCHES "same70")
    add_custom_target(flash-${PROJECT_NAME}
        COMMAND openocd
            -f board/atmel_same70_xplained.cfg
            -c "program ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf verify reset exit"
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Flashing ${PROJECT_NAME} to SAME70 Xplained"
    )
endif()
