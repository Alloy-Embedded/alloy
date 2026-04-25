# Alloy driver seed library
#
# Drivers are header-only and templated over the public HAL handle types. This interface
# target just exposes the `drivers/` include root so applications can pull headers with
# `#include "drivers/display/ssd1306/ssd1306.hpp"`.

add_library(alloy-drivers INTERFACE)
add_library(alloy::drivers ALIAS alloy-drivers)

target_include_directories(alloy-drivers INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(alloy-drivers INTERFACE alloy::alloy)

# Protocol drivers may carry implementation files (state machines, codecs) and
# therefore live as proper static libraries below. The interface target above
# stays for the header-only seed drivers.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/protocol/modbus/CMakeLists.txt")
    add_subdirectory(protocol/modbus)
endif()

# Network abstraction layer: concept headers + optional lwIP integration.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/net/CMakeLists.txt")
    add_subdirectory(net)
endif()
