cmake_minimum_required(VERSION 3.25)

# Tiny downstream-consumer smoke test for the alloy CMake surface.
#
# Run with:
#
#     cmake -S tests/cmake/fetchcontent_smoke \
#           -B build/fetchcontent-smoke \
#           -DALLOY_SOURCE_DIR=${PWD}                \
#           -DALLOY_BOARD=host
#     cmake --build build/fetchcontent-smoke
#
# We use add_subdirectory directly (no FetchContent_Declare network
# call) so the test runs offline; this is the same code path
# FetchContent_MakeAvailable executes once it's downloaded the
# repo.

project(alloy_fetchcontent_smoke LANGUAGES CXX)

if(NOT DEFINED ALLOY_SOURCE_DIR)
    message(FATAL_ERROR
        "fetchcontent_smoke needs -DALLOY_SOURCE_DIR=<path-to-alloy-checkout>")
endif()

# Mirror the configure-time defaults a real downstream consumer
# would set when consuming alloy via FetchContent.  We pin
# ALLOY_BOARD=host so the smoke can configure without a
# cross-toolchain.
if(NOT DEFINED ALLOY_BOARD)
    set(ALLOY_BOARD "host" CACHE STRING "")
endif()

# Disable alloy's heavyweight test + example targets — the smoke
# only cares that the helper loads + the alias target exists.
set(ALLOY_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(ALLOY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALLOY_BUILD_DRIVERS  OFF CACHE BOOL "" FORCE)

add_subdirectory("${ALLOY_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_alloy_subbuild")

# ---------------------------------------------------------------------------
# Asserts that the surface we promised downstream consumers exists.
# ---------------------------------------------------------------------------

if(NOT TARGET Alloy::hal)
    message(FATAL_ERROR
        "fetchcontent_smoke: target Alloy::hal is missing — did "
        "alloy/CMakeLists.txt forget the alias?")
endif()

if(NOT TARGET alloy::hal)
    message(FATAL_ERROR
        "fetchcontent_smoke: lowercase alias alloy::hal is missing.")
endif()

if(NOT COMMAND alloy_add_runtime_executable)
    message(FATAL_ERROR
        "fetchcontent_smoke: alloy_add_runtime_executable is not "
        "defined — AlloyHelpers.cmake didn't load via add_subdirectory.")
endif()

# Build a tiny app via the helper — the whole point of the smoke.
alloy_add_runtime_executable(downstream_smoke
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

message(STATUS "fetchcontent_smoke OK — Alloy::hal + helper both available.")
