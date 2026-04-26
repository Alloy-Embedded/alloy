# Espressif ESP32-S3 Platform Configuration
# Architecture: Xtensa LX7 (dual-core, alloy targets core 0)
# Toolchain: xtensa-esp32s3-elf-gcc (ESP-IDF or standalone)

message(STATUS "Configuring for ESP32-S3 platform (Xtensa LX7)")

set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/device)

add_compile_options(-mlongcalls
                    -Wall -Wextra -ffunction-sections -fdata-sections
                    -fno-exceptions -fno-rtti -fno-threadsafe-statics)
add_link_options(-mlongcalls -Wl,--gc-sections)

add_compile_definitions(ALLOY_SINGLE_CORE=0)

message(STATUS "ESP32-S3 platform configured")
