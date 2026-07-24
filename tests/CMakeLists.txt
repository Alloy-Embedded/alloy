# Host unit tests for alloy's portable layers. Authored (not generated), so it
# lives outside .alloy/ and is a legitimate hand-written artifact under the
# NORTH_STAR contract. Native toolchain, no board data, no linker script — it
# compiles ONLY the arch/board-free code (the Tier-1 utilities/services + their
# test-doubles). Compiled with the firmware's -fno-exceptions/-fno-rtti so the
# exact same code that runs here also cross-compiles onto silicon.
cmake_minimum_required(VERSION 3.25)
# C is enabled for the vendored littlefs sources (fs facade tests); everything
# authored here is C++23.
project(alloy_tests C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

# -fno-exceptions/-fno-rtti are C++-only — gate them so the vendored C compiles.
add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    -Wall -Wextra -Werror -g -O1)

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

# Vendored littlefs (C) — compiled into the suite so the fs facade is exercised
# for real. Silence its warnings (the suite is -Werror); asserts + sanitizers
# stay ON to catch filesystem bugs.
set(ALLOY_LFS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/alloy/fs/vendor/lfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/alloy/fs/vendor/lfs_util.c)
set_source_files_properties(${ALLOY_LFS} PROPERTIES COMPILE_OPTIONS "-w")

add_executable(alloy_unit_tests
    main.cpp
    host_support.cpp
    test_util.cpp
    test_control.cpp
    test_services.cpp
    test_irq.cpp
    test_async.cpp
    test_units.cpp
    test_nvm.cpp
    test_gpio_bus.cpp
    test_dsp.cpp
    test_secure.cpp
    test_fs.cpp
    ${ALLOY_LFS})

# The runner's exit code is the number of failing tests (0 = pass).
add_test(NAME alloy_unit_tests COMMAND alloy_unit_tests)
