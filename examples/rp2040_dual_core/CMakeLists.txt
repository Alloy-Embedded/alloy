cmake_minimum_required(VERSION 3.25)

project(rp2040_dual_core
    VERSION 1.0.0
    DESCRIPTION "RP2040 dual-core demo — CrossCoreChannel between Cortex-M0+ cores"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOARD_DIR "${CMAKE_SOURCE_DIR}/boards/raspberry_pi_pico")

set(SOURCES
    main.cpp
    "${BOARD_DIR}/startup.cpp"
    "${BOARD_DIR}/board_multicore.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/boards"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ALLOY_SINGLE_CORE=0)

set(CPU_FLAGS -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -Wall -Wextra
    -ffunction-sections -fdata-sections
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>
)

target_link_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

set(_LD "${CMAKE_SOURCE_DIR}/boards/raspberry_pi_pico/rp2040.ld")
if(EXISTS "${_LD}")
    target_link_options(${PROJECT_NAME} PRIVATE -T${_LD})
endif()

# Convert ELF to UF2 for Pico drag-and-drop flashing
find_program(_ELF2UF2 elf2uf2)
if(_ELF2UF2)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${_ELF2UF2}
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.uf2"
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Converting to UF2 for Pico drag-and-drop flashing"
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "elf2uf2 not found — convert manually or use picotool"
    )
endif()

message(STATUS "rp2040_dual_core: board=raspberry_pi_pico, ld=${_LD}")
