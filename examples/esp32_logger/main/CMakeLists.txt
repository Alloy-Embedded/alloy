# Main component for ESP32 Logger example

set(SRCS
    "main.cpp"
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS
        "."
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../src"
    REQUIRES
        esp_system
        driver
        nvs_flash
)

# Set component compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -DESP_PLATFORM
    -DALLOY_HAS_RTOS
    # Logger configuration
    -DLOG_MIN_LEVEL=LOG_LEVEL_TRACE
    -DLOG_ENABLE_TIMESTAMPS=1
    -DLOG_ENABLE_SOURCE_LOCATION=1
)
