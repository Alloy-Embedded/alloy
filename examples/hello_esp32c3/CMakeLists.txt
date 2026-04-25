cmake_minimum_required(VERSION 3.25)

project(hello_esp32c3
    VERSION 1.0.0
    DESCRIPTION "ESP32-C3 bare-metal bring-up — direct boot, UART hello, LED blink"
    LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOARD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/boards/esp32c3_devkitm")

set(SOURCES
    main.cpp
    "${BOARD_SOURCE_DIR}/board.cpp"
    "${BOARD_SOURCE_DIR}/startup.cpp"
    "${BOARD_SOURCE_DIR}/syscalls.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/boards"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
    -ffunction-sections -fdata-sections
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>
)

target_link_options(${PROJECT_NAME} PRIVATE
    -nostartfiles
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -lc -lm -lgcc
)

# Linker script (applied by esp32c3.cmake platform, but also safe to restate here)
set(_LD "${CMAKE_SOURCE_DIR}/boards/esp32c3_devkitm/esp32c3.ld")
if(EXISTS "${_LD}")
    target_link_options(${PROJECT_NAME} PRIVATE -T${_LD})
endif()

# Post-build: generate .bin for esptool
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${PROJECT_NAME}.bin"
    COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating ${PROJECT_NAME}.bin for esptool"
)

message(STATUS "hello_esp32c3: board=${ALLOY_BOARD}, ld=${_LD}")
