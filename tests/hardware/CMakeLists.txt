# ==============================================================================
# Alloy HAL Hardware Validation Tests
# ==============================================================================
#
# Hardware validation tests run on actual embedded hardware.
# These tests are ONLY built when targeting embedded platforms.
#

# Check if we're building for embedded hardware
if(NOT ALLOY_PLATFORM STREQUAL "host" AND NOT ALLOY_PLATFORM STREQUAL "linux")
    message(STATUS "Hardware validation tests enabled for platform: ${ALLOY_PLATFORM}")

    # ==============================================================================
    # Hardware Test Configuration
    # ==============================================================================

    # Hardware tests don't use Catch2 - they use simple assertions
    # that can run without hosted environment (no printf, no dynamic allocation)

    function(add_hardware_test TEST_NAME)
        cmake_parse_arguments(ARG "" "BOARD" "SOURCES" ${ARGN})

        add_executable(${TEST_NAME} ${ARG_SOURCES})

        target_include_directories(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/tests
        )

        target_compile_options(${TEST_NAME} PRIVATE
            -std=c++20
            -Wall
            -Wextra
            -Os  # Optimize for size on embedded
        )

        # Link with HAL platform sources
        target_link_libraries(${TEST_NAME} PRIVATE
            # Platform-specific libraries would go here
        )

        # Set linker script if provided by board
        if(EXISTS ${CMAKE_SOURCE_DIR}/boards/${ARG_BOARD}/linker.ld)
            target_link_options(${TEST_NAME} PRIVATE
                -T${CMAKE_SOURCE_DIR}/boards/${ARG_BOARD}/linker.ld
            )
        endif()

        message(STATUS "  + Hardware test: ${TEST_NAME} (${ARG_BOARD})")
    endfunction()

    # ==============================================================================
    # Hardware Tests
    # ==============================================================================

    # Test 1: GPIO LED Blink (validates GPIO + Clock)
    add_hardware_test(hw_gpio_led_test
        BOARD ${ALLOY_BOARD}
        SOURCES hw_gpio_led_test.cpp
    )

    # Test 2: Clock Frequency Validation
    add_hardware_test(hw_clock_validation
        BOARD ${ALLOY_BOARD}
        SOURCES hw_clock_validation.cpp
    )

    # Test 3: Board-specific validation
    add_hardware_test(hw_board_validation
        BOARD ${ALLOY_BOARD}
        SOURCES hw_board_validation.cpp
    )

    message(STATUS "Hardware validation tests configured for ${ALLOY_BOARD}")

else()
    message(STATUS "Hardware validation tests skipped (host platform)")
endif()
