# Host unit tests for alloy's portable layers. Authored (not generated), so it
# lives outside .alloy/ and is a legitimate hand-written artifact under the
# NORTH_STAR contract. Native toolchain, no board data, no linker script — it
# compiles ONLY the arch/board-free code (the Tier-1 utilities/services + their
# test-doubles). Compiled with the firmware's -fno-exceptions/-fno-rtti so the
# exact same code that runs here also cross-compiles onto silicon.
cmake_minimum_required(VERSION 3.25)
project(alloy_tests CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-fno-exceptions -fno-rtti -Wall -Wextra -Werror -g -O1)

# Sanitizers catch real bugs on the host build, but ASan's at-exit pass hangs
# on macOS/arm64 — default them ON on Linux (where CI runs) and OFF on Apple.
if(APPLE)
    set(_alloy_san_default OFF)
else()
    set(_alloy_san_default ON)
endif()
option(ALLOY_TEST_SANITIZE "Build host tests with AddressSanitizer + UBSan" ${_alloy_san_default})
if(ALLOY_TEST_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# alloy headers resolve as <alloy/...>.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

enable_testing()

add_executable(alloy_unit_tests
    main.cpp
    host_support.cpp
    test_util.cpp
    test_control.cpp
    test_services.cpp)

# The runner's exit code is the number of failing tests (0 = pass).
add_test(NAME alloy_unit_tests COMMAND alloy_unit_tests)
