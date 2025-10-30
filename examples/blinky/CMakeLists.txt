# Blinky Example
# Simple LED blinking example demonstrating GPIO usage

add_executable(blinky
    main.cpp
)

target_include_directories(blinky
    PRIVATE
        ${ALLOY_ROOT}/src
)

target_link_libraries(blinky
    PRIVATE
        alloy_hal_host
)

target_compile_features(blinky
    PRIVATE
        cxx_std_20
)

# Apply Alloy compiler options
alloy_target_compile_options(blinky)
