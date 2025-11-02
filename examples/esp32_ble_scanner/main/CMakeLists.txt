# Main component for BLE Scanner example

set(SRCS "main.cpp")
set(ALLOY_BLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/ble")

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "." "${CMAKE_CURRENT_SOURCE_DIR}/../../../src"
    REQUIRES esp_system bt nvs_flash
)

# Add Alloy source files
target_sources(${COMPONENT_LIB} PRIVATE
    "${ALLOY_BLE_PATH}/central.cpp"
    "${ALLOY_BLE_PATH}/peripheral.cpp"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -DESP32)
