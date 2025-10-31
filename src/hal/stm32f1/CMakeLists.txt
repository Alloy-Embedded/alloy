# Alloy HAL: STM32F1 (Blue Pill)

add_library(alloy_hal_stm32f1 STATIC
    gpio.cpp
    uart.cpp
)

target_include_directories(alloy_hal_stm32f1 PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Include generated peripheral definitions if available
if(ALLOY_GENERATED_DIR AND EXISTS "${ALLOY_GENERATED_DIR}/peripherals.hpp")
    target_include_directories(alloy_hal_stm32f1 PUBLIC
        ${ALLOY_GENERATED_DIR}
    )
    target_compile_definitions(alloy_hal_stm32f1 PUBLIC
        ALLOY_USE_GENERATED_PERIPHERALS
    )
    message(STATUS "  Using generated peripheral definitions from ${ALLOY_GENERATED_DIR}")
else()
    message(STATUS "  Using hardcoded peripheral definitions (fallback)")
endif()

target_compile_options(alloy_hal_stm32f1 PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -Wall
    -Wextra
    -Werror
)

target_compile_features(alloy_hal_stm32f1 PUBLIC
    cxx_std_20
)

# Link with core library
target_link_libraries(alloy_hal_stm32f1 PUBLIC
    alloy_core
)

# Alias for consistent naming
add_library(alloy::hal::stm32f1 ALIAS alloy_hal_stm32f1)

message(STATUS "HAL: STM32F1 configured")
