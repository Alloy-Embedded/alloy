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
