# Alloy HAL Interface (header-only)

add_library(alloy_hal_interface INTERFACE)

target_include_directories(alloy_hal_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(alloy_hal_interface INTERFACE
    cxx_std_20
)

# Link with core library for error types
target_link_libraries(alloy_hal_interface INTERFACE
    alloy_core
)

# Alias for consistent naming
add_library(alloy::hal::interface ALIAS alloy_hal_interface)
