# Microchip AVR-DA Platform Configuration
# Architecture: AVR 8-bit (AVR128DA32, etc.)
# NOTE: AVR-GCC uses -mmcu=<device> to select the target. The board CMakeLists
# sets ALLOY_MCU which maps to the avr-gcc -mmcu value.

message(STATUS "Configuring for AVR-DA platform (AVR8)")

set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/device)

# Map ALLOY_MCU to avr-gcc -mmcu string
if(DEFINED ALLOY_MCU)
    string(TOLOWER "${ALLOY_MCU}" _avr_mcu)
else()
    set(_avr_mcu "avr128da32")
endif()

set(AVR_MCU_FLAGS -mmcu=${_avr_mcu})
add_compile_options(${AVR_MCU_FLAGS})
add_link_options(${AVR_MCU_FLAGS})

add_compile_options(-Wall -Wextra -ffunction-sections -fdata-sections
                    -fno-exceptions -fno-rtti -fno-threadsafe-statics)
add_link_options(-Wl,--gc-sections)

add_compile_definitions(ALLOY_SINGLE_CORE=1)

message(STATUS "AVR-DA platform configured: -mmcu=${_avr_mcu}")
