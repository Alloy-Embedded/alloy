# Espressif ESP32 Platform Configuration
# Architecture: Xtensa LX6 dual-core, bare-metal app loaded by ESP-IDF bootloader

message(STATUS "Configuring for ESP32 platform (Xtensa LX6)")

set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/device)

add_compile_options(
    -mlongcalls
    -Wall -Wextra
    -ffunction-sections -fdata-sections
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
)

# App linker script (IRAM + DRAM, no flash mapping — bootloader copies everything)
set(_ESP32_LD "${CMAKE_CURRENT_SOURCE_DIR}/boards/esp32_devkit/esp32.ld")
if(EXISTS "${_ESP32_LD}")
    add_link_options(
        -mlongcalls
        -nostartfiles
        -T${_ESP32_LD}
        -Wl,--gc-sections
        -Wl,--print-memory-usage
    )
    message(STATUS "  Linker script : ${_ESP32_LD}")
else()
    add_link_options(-mlongcalls -Wl,--gc-sections)
    message(WARNING "esp32.ld not found")
endif()

message(STATUS "ESP32 platform configured")
