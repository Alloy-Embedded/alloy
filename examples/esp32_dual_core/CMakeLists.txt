cmake_minimum_required(VERSION 3.25)

project(esp32_dual_core
    VERSION 1.0.0
    DESCRIPTION "ESP32 dual-core demo — CrossCoreChannel between PRO_CPU and APP_CPU"
    LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOARD_DIR "${CMAKE_SOURCE_DIR}/boards/esp32_devkit")

set(SOURCES
    main.cpp
    "${BOARD_DIR}/board.cpp"
    "${BOARD_DIR}/startup.S"
    "${BOARD_DIR}/startup.cpp"
    "${BOARD_DIR}/syscalls.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/boards"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ALLOY_SINGLE_CORE=0)

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

find_program(_ESPTOOL esptool.py esptool)
if(_ESPTOOL)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${_ESPTOOL} --chip esp32
            elf2image
            --dont-append-digest
            --output "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating ${PROJECT_NAME}.bin via esptool elf2image"
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "esptool not found — run: esptool.py --chip esp32 elf2image esp32_dual_core"
    )
endif()

message(STATUS "esp32_dual_core: board=esp32_devkit, ld=${_LD}")
