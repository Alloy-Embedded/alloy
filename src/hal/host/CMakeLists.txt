# Host HAL Library
# Provides mockable hardware abstraction for development on PC

add_library(alloy_hal_host STATIC
    gpio.cpp
)

target_include_directories(alloy_hal_host
    PUBLIC
        ${ALLOY_ROOT}/src
)

target_compile_features(alloy_hal_host
    PUBLIC
        cxx_std_20
)

# Link with interface headers (header-only, no library)
target_include_directories(alloy_hal_host
    PUBLIC
        ${ALLOY_ROOT}/src/hal/interface
)

# Apply Alloy compiler options
alloy_target_compile_options(alloy_hal_host)
