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

# alloy headers resolve as <alloy/...>; the shared test harness (alloy_test.hpp)
# resolves for both in-tree tests and libs/*/tests via the tests dir itself.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Ecosystem libraries (libs/): every reference driver's test runs here against
# the testkit mock bus, so a driver that regresses can't ship. testkit/ resolves
# via ../libs; each driver's own header via its include/ dir. Glob so a newly
# added driver joins the suite with no edit here.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libs)
file(GLOB _alloy_lib_includes ${CMAKE_CURRENT_SOURCE_DIR}/../libs/*/include)
foreach(_inc ${_alloy_lib_includes})
    include_directories(${_inc})
endforeach()
file(GLOB ALLOY_LIB_TESTS ${CMAKE_CURRENT_SOURCE_DIR}/../libs/*/tests/test_*.cpp)

enable_testing()

# Vendored littlefs (C) — compiled into the suite so the fs facade is exercised
# for real. Silence its warnings (the suite is -Werror); asserts + sanitizers
# stay ON to catch filesystem bugs.
set(ALLOY_LFS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/alloy/fs/vendor/lfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/alloy/fs/vendor/lfs_util.c)
set_source_files_properties(${ALLOY_LFS} PROPERTIES COMPILE_OPTIONS "-w")

# The host net tests exercise the Socket facade over lwIP's loopback (127.0.0.1).
# lwipopts.h is GENERATED from data (host profile: loopback on) by `alloy test`
# before it configures this build, and handed in via ALLOY_LWIPOPTS_DIR — the same
# generator the firmware uses, so host and target can't drift. Run the host tests
# through `alloy test`, not a bare cmake, so that header exists.
if(NOT DEFINED ALLOY_LWIPOPTS_DIR)
    message(FATAL_ERROR
        "ALLOY_LWIPOPTS_DIR not set — run the host tests via `alloy test`, which "
        "generates lwipopts.h from data before configuring this build.")
endif()

# Vendored Monocypher (C) — compiled in so the SIGNED image path and the key
# rotation ring are host-testable with real Ed25519 keys and real signatures.
# Without this the only proof of the update root of trust was the emulation leg,
# which cannot run the negative cases cheaply. Warnings off on vendored code.
set(ALLOY_MONOCYPHER
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/monocypher/monocypher.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/monocypher/monocypher-ed25519.c)
set_source_files_properties(${ALLOY_MONOCYPHER} PROPERTIES COMPILE_OPTIONS "-w")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/monocypher)

# Vendored lwIP (C) — the M2 net stack facade is exercised over the loopback.
# The generated lwipopts.h leads; arch/cc.h is ours; the vendored lwIP headers
# are SYSTEM (no -Werror on vendored code) and its .c get -w.
set(_lwip_root ${CMAKE_CURRENT_SOURCE_DIR}/../src/alloy/net)
include_directories(${ALLOY_LWIPOPTS_DIR})
include_directories(${_lwip_root}/lwip)
include_directories(SYSTEM ${_lwip_root}/vendor/lwip/src/include)
file(GLOB ALLOY_LWIP
    ${_lwip_root}/vendor/lwip/src/core/*.c
    ${_lwip_root}/vendor/lwip/src/core/ipv4/*.c)
list(APPEND ALLOY_LWIP ${_lwip_root}/vendor/lwip/src/netif/ethernet.c)
set_source_files_properties(${ALLOY_LWIP} PROPERTIES COMPILE_OPTIONS "-w")

add_executable(alloy_unit_tests
    main.cpp
    host_support.cpp
    test_mmio.cpp
    test_rl78_intc.cpp
    test_util.cpp
    test_uptime_us.cpp
    test_control.cpp
    test_services.cpp
    test_irq.cpp
    test_async.cpp
    test_sched.cpp
    test_units.cpp
    test_lpuart_baud.cpp
    test_claim.cpp
    # A second TU that names the same instances as test_claim.cpp — the only
    # arrangement that can tell an inline variable template apart from a
    # per-compilation static. It holds no ALLOY_TESTs of its own.
    test_claim_tu2.cpp
    test_encoder.cpp
    test_bridge.cpp
    test_tick.cpp
    test_nvm.cpp
    test_gpio_bus.cpp
    test_dsp.cpp
    test_fsm.cpp
    test_secure.cpp
    test_fs.cpp
    test_ota.cpp
    test_ota_signed.cpp
    test_ota_transport.cpp
    test_provision.cpp
    test_can.cpp
    test_dma_ring.cpp
    test_st_dma_v1_latch.cpp
    test_st_dma_v2_latch.cpp
    test_st_usart_v4_idle.cpp
    test_adc_stream.cpp
    test_uart_rx_stream.cpp
    test_net.cpp
    test_wwdt.cpp
    test_crc.cpp
    test_uid.cpp
    ${ALLOY_LIB_TESTS}
    ${ALLOY_LFS}
    ${ALLOY_LWIP}
    ${ALLOY_MONOCYPHER})

# The runner's exit code is the number of failing tests (0 = pass).
add_test(NAME alloy_unit_tests COMMAND alloy_unit_tests)
