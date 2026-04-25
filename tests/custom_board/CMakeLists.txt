cmake_minimum_required(VERSION 3.25)

# Synthetic external project that exercises the ALLOY_BOARD=custom branch in
# cmake/board_manifest.cmake. The expectation is that this configures cleanly
# against a real alloy-devices descriptor (the script feeds in the
# st/stm32g0/stm32g071rb tuple by default), proving that a board declared
# outside the runtime tree flows through the same descriptor pipeline as the
# in-tree foundational boards.

set(ALLOY_BOARD "custom" CACHE STRING "" FORCE)

# Allow the negative-test driver to omit a required variable so the manifest's
# fail-fast path is exercised. Set ALLOY_CUSTOM_OMIT=ALLOY_CUSTOM_BOARD_HEADER
# (or any other required name) to skip the corresponding default below.
if(NOT "${ALLOY_CUSTOM_OMIT}" STREQUAL "ALLOY_CUSTOM_BOARD_HEADER")
    set(ALLOY_CUSTOM_BOARD_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/board.hpp"
        CACHE FILEPATH "" FORCE)
endif()
if(NOT "${ALLOY_CUSTOM_OMIT}" STREQUAL "ALLOY_CUSTOM_LINKER_SCRIPT")
    set(ALLOY_CUSTOM_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/custom.ld"
        CACHE FILEPATH "" FORCE)
endif()
set(ALLOY_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Required device tuple. Defaults match a known descriptor for the convenience
# of the smoke script; the same project file will work for any descriptor by
# overriding these on the configure command line.
if(NOT DEFINED ALLOY_DEVICE_VENDOR)
    set(ALLOY_DEVICE_VENDOR "st" CACHE STRING "" FORCE)
endif()
if(NOT DEFINED ALLOY_DEVICE_FAMILY)
    set(ALLOY_DEVICE_FAMILY "stm32g0" CACHE STRING "" FORCE)
endif()
if(NOT DEFINED ALLOY_DEVICE_NAME)
    set(ALLOY_DEVICE_NAME "stm32g071rb" CACHE STRING "" FORCE)
endif()
if(NOT DEFINED ALLOY_DEVICE_ARCH)
    set(ALLOY_DEVICE_ARCH "cortex-m0plus" CACHE STRING "" FORCE)
endif()

if(NOT DEFINED ALLOY_ROOT)
    message(FATAL_ERROR
        "Set -DALLOY_ROOT=<path> to the alloy runtime when configuring this test.")
endif()

project(alloy_custom_board_smoke LANGUAGES C CXX ASM)

add_subdirectory(${ALLOY_ROOT} ${CMAKE_BINARY_DIR}/_alloy)

# Configure success alone is the smoke check; we do not produce a firmware
# image here because the test runs without a cross-toolchain in CI.
add_custom_target(alloy_custom_board_smoke ALL
    COMMAND ${CMAKE_COMMAND} -E echo
        "alloy custom-board smoke OK: ${ALLOY_DEVICE_VENDOR}/${ALLOY_DEVICE_FAMILY}/${ALLOY_DEVICE_NAME}"
)
