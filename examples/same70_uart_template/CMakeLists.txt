# ==============================================================================
# SAME70 Template UART Example
# ==============================================================================
#
# This example demonstrates the template-based UART implementation with
# ZERO virtual functions and ZERO runtime overhead.
#
# Build:
#   cmake -DALLOY_BOARD=same70_xpld \
#         -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/arm-none-eabi.cmake \
#         -B build-same70
#   make -C build-same70
#
# Flash:
#   openocd -f board/atmel_same70_xplained.cfg \
#           -c "program build-same70/same70_uart_template.elf verify reset exit"
#
# ==============================================================================

cmake_minimum_required(VERSION 3.25)

# Example name
set(EXAMPLE_NAME same70_uart_template)

# Create executable
add_executable(${EXAMPLE_NAME}
    main.cpp
)

# Link with HAL library (created by root CMakeLists.txt)
target_link_libraries(${EXAMPLE_NAME}
    PRIVATE
        alloy-hal
)

# Add board include path
target_include_directories(${EXAMPLE_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/boards/atmel_same70_xpld
)

# Startup code (board-specific)
target_sources(${EXAMPLE_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/boards/atmel_same70_xpld/startup.cpp
)

# Set linker script
if(EXISTS ${CMAKE_SOURCE_DIR}/boards/atmel_same70_xpld/ATSAME70Q21.ld)
    target_link_options(${EXAMPLE_NAME}
        PRIVATE
            -T${CMAKE_SOURCE_DIR}/boards/atmel_same70_xpld/ATSAME70Q21.ld
    )
endif()

# Generate .bin and .hex files for flashing
add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${EXAMPLE_NAME}.elf ${EXAMPLE_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXAMPLE_NAME}.elf ${EXAMPLE_NAME}.hex
    COMMENT "Generating ${EXAMPLE_NAME}.bin and ${EXAMPLE_NAME}.hex"
)

# Print size information
add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} ${EXAMPLE_NAME}.elf
    COMMENT "Size of ${EXAMPLE_NAME}:"
)

# Print build summary
message(STATUS "Example: ${EXAMPLE_NAME}")
message(STATUS "  Platform: ${ALLOY_PLATFORM}")
message(STATUS "  Board: ${ALLOY_BOARD}")
