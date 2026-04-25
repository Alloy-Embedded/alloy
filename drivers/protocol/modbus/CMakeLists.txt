# alloy::modbus -- in-tree Modbus protocol library.
#
# Three-layer architecture (see openspec/changes/add-modbus-protocol):
#   1. PDU + framing (pure logic; no I/O; testable on host)
#   2. byte_stream transport (UART today, TCP scaffolded for the future network HAL)
#   3. variable registry + master/slave (typed `var<T>` + cooperative scheduling)
#
# This CMakeLists builds layer 1 today; later phases add the framing module, the
# transport adapters, the variable registry, and the master/slave implementations.

set(_alloy_modbus_sources
    src/pdu.cpp
    src/rtu_frame.cpp
)

add_library(alloy-modbus STATIC ${_alloy_modbus_sources})
add_library(alloy::modbus ALIAS alloy-modbus)

target_include_directories(alloy-modbus PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# core::Result lives under src/core. The HAL target republishes the runtime
# include path; downstream consumers of alloy::modbus pick up `core/result.hpp`
# transitively.
target_link_libraries(alloy-modbus PUBLIC Alloy::hal)

target_compile_features(alloy-modbus PUBLIC cxx_std_23)

target_compile_options(alloy-modbus PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if(ALLOY_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
