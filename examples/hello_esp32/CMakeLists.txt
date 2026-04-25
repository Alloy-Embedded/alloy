cmake_minimum_required(VERSION 3.25)

project(hello_esp32
    VERSION 1.0.0
    DESCRIPTION "ESP32-DevKit bare-metal bring-up — ESP-IDF bootloader, UART hello, LED blink"
    LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOARD_DIR "${CMAKE_SOURCE_DIR}/boards/esp32_devkit")

set(SOURCES
    main.cpp
    "${BOARD_DIR}/board.cpp"
    "${BOARD_DIR}/startup.S"      # Xtensa window init + stack setup
    "${BOARD_DIR}/startup.cpp"    # BSS/data/ctors/main
    "${BOARD_DIR}/syscalls.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/boards"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -mlongcalls
    -Wall -Wextra
    -ffunction-sections -fdata-sections
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>
)

target_link_options(${PROJECT_NAME} PRIVATE
    -mlongcalls
    -nostartfiles
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -lc -lm -lgcc
)

set(_LD "${CMAKE_SOURCE_DIR}/boards/esp32_devkit/esp32.ld")
if(EXISTS "${_LD}")
    target_link_options(${PROJECT_NAME} PRIVATE -T${_LD})
endif()

# Post-build: generate ESP32 image binary via esptool elf2image
find_program(_ESPTOOL esptool.py esptool)
if(_ESPTOOL)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${_ESPTOOL} --chip esp32
            elf2image
            --output "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating ${PROJECT_NAME}.bin via esptool elf2image"
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "esptool not found — run manually: esptool.py --chip esp32 elf2image hello_esp32"
    )
endif()

message(STATUS "hello_esp32: board=esp32_devkit, ld=${_LD}")
