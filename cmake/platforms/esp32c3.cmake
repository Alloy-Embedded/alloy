# Espressif ESP32-C3 Platform Configuration
# Architecture: RISC-V RV32IMC, direct boot (no second-stage bootloader)
# Toolchain: riscv32-esp-elf-gcc (alloy-managed or ESP-IDF)

message(STATUS "Configuring for ESP32-C3 platform (RISC-V RV32IMC, direct boot)")

set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/device)

add_compile_options(
    -march=rv32imc -mabi=ilp32
    -Wall -Wextra
    -ffunction-sections -fdata-sections
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
)

# Direct boot linker script
set(_ESP32C3_LD "${CMAKE_CURRENT_SOURCE_DIR}/boards/esp32c3_devkitm/esp32c3.ld")
if(EXISTS "${_ESP32C3_LD}")
    add_link_options(
        -march=rv32imc -mabi=ilp32
        -nostartfiles
        -T${_ESP32C3_LD}
        -Wl,--gc-sections
        -Wl,--print-memory-usage
    )
    message(STATUS "  Linker script : ${_ESP32C3_LD}")
else()
    add_link_options(-march=rv32imc -mabi=ilp32 -Wl,--gc-sections)
    message(WARNING "esp32c3.ld not found — linker script not applied")
endif()

add_compile_definitions(ALLOY_SINGLE_CORE=1)

message(STATUS "ESP32-C3 platform configured (direct boot)")
