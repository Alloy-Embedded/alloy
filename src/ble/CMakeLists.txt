# BLE Component for Alloy Framework
#
# Provides BLE (Bluetooth Low Energy) abstractions for ESP32

set(SRCS
    "central.cpp"
    "peripheral.cpp"
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "." "../core"
    REQUIRES bt nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=c++20
    -Wall
    -Wextra
    -DESP32
)
