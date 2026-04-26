cmake_minimum_required(VERSION 3.25)

project(filesystem_littlefs
    VERSION 1.0.0
    DESCRIPTION "LittleFS persistent counter on W25Q128 NOR flash — nucleo_g071rb"
    LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES main.cpp)

if(DEFINED ALLOY_BOARD_SOURCE_DIR)
    set(BOARD_SOURCE_DIR "${ALLOY_BOARD_SOURCE_DIR}")
else()
    set(BOARD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/boards/${ALLOY_BOARD}")
endif()

if(EXISTS "${BOARD_SOURCE_DIR}/board.cpp")
    list(APPEND SOURCES "${BOARD_SOURCE_DIR}/board.cpp")
endif()

if(CMAKE_CROSSCOMPILING AND EXISTS "${BOARD_SOURCE_DIR}/syscalls.cpp")
    list(APPEND SOURCES "${BOARD_SOURCE_DIR}/syscalls.cpp")
endif()

if(CMAKE_CROSSCOMPILING AND DEFINED STARTUP_SOURCE AND NOT DEFINED ALLOY_HAL_LIBRARY)
    list(APPEND SOURCES ${STARTUP_SOURCE})
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
    ${CMAKE_SOURCE_DIR}/drivers
    ${CMAKE_SOURCE_DIR}/examples
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${ALLOY_HAL_LIBRARY}
    alloy::littlefs
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O2 -g>
    $<$<CONFIG:MinSizeRel>:-Os -g>
)

if(CMAKE_CROSSCOMPILING)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -ffunction-sections -fdata-sections
        -fno-exceptions -fno-rtti -fno-threadsafe-statics
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs --specs=nosys.specs
    )
    if(DEFINED LINKER_SCRIPT AND EXISTS "${LINKER_SCRIPT}")
        target_link_options(${PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HEX and BIN for ${PROJECT_NAME}"
    )
endif()
