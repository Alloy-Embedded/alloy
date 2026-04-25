# Raspberry Pi Pico (RP2040) Platform Configuration
# Architecture: ARMv6-M Cortex-M0+ (dual-core, alloy targets core 0 only)
# Clock: 12 MHz XOSC → PLL → 125 MHz system clock

message(STATUS "Configuring for RP2040 platform (Cortex-M0+)")

set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/device)

set(RP2040_CPU_FLAGS -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft)
add_compile_options(${RP2040_CPU_FLAGS})
add_link_options(${RP2040_CPU_FLAGS} --specs=nosys.specs)

add_compile_options(-Wall -Wextra -ffunction-sections -fdata-sections
                    -fno-exceptions -fno-rtti -fno-threadsafe-statics)
add_link_options(-Wl,--gc-sections)

message(STATUS "RP2040 platform configured")
