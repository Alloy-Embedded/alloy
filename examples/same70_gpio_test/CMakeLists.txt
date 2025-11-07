# ==============================================================================
# SAME70 GPIO Test Example
# ==============================================================================
#
# Demonstrates template-based GPIO with zero overhead:
# - LED control with compile-time pin masks
# - Button input with pull-up and filter
# - Type-safe pin access
#
# Build:
#   cmake -DALLOY_BOARD=same70_xpld -B build-same70
#   make -C build-same70 same70_gpio_test
#
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

# Executable
add_executable(same70_gpio_test
    main.cpp
)

# Link HAL library
target_link_libraries(same70_gpio_test
    PRIVATE
        alloy-hal
)

# Include directories
target_include_directories(same70_gpio_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
)

# Set C++20
set_target_properties(same70_gpio_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Flash target
if(COMMAND add_flash_target)
    add_flash_target(same70_gpio_test)
endif()

message(STATUS "Configured example: same70_gpio_test (SAME70 platform)")
