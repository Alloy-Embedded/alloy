cmake_minimum_required(VERSION 3.25)

# lite_hal_probe — integration smoke-test for the alloy.device.v2.1 lite HAL.
#
# Exercises GPIO + UART + SPI + I2C + Timer in a single binary using only the
# direct-MMIO concept-gated drivers (hal/*/lite.hpp) and the generated
# rcc_enable.hpp clock-gate helpers.  No legacy descriptor-runtime required.
#
# Target: Nucleo-G071RB (STM32G071RBT6) with HSI16 (16 MHz default clock).
#
# Prerequisites — populate alloy/device/ before configuring:
#   cd alloy-codegen
#   python3 ... st/stm32g0/stm32g071rb \
#       --emit peripheral_traits --emit peripheral_id --emit rcc_enable \
#       --out ../alloy/device

project(lite_hal_probe
    VERSION 1.0.0
    DESCRIPTION "alloy.device.v2.1 lite HAL integration probe"
    LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Sources ──────────────────────────────────────────────────────────────────

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

# ── Executable ───────────────────────────────────────────────────────────────

add_executable(${PROJECT_NAME} ${SOURCES})

# ── Include paths ─────────────────────────────────────────────────────────────

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src       # hal/, device/, core/
    ${CMAKE_SOURCE_DIR}/boards
)

# ALLOY_DEVICE_INCLUDE_DIRS is set by alloy_configure_selected_device() and
# includes the alloy/device/ tree (v2.1 codegen output) + generated/ dir.
if(DEFINED ALLOY_DEVICE_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ALLOY_DEVICE_INCLUDE_DIRS})
endif()

# ── HAL library (optional; used when building via the top-level project) ─────

if(DEFINED ALLOY_HAL_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ALLOY_HAL_LIBRARY})
endif()

# ── Compiler flags ────────────────────────────────────────────────────────────

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O2 -g>
    $<$<CONFIG:MinSizeRel>:-Os -g>
)

if(CMAKE_CROSSCOMPILING)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
    )

    if(DEFINED LINKER_SCRIPT AND EXISTS "${LINKER_SCRIPT}")
        target_link_options(${PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_SIZE}              $<TARGET_FILE:${PROJECT_NAME}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HEX/BIN for ${PROJECT_NAME}"
    )

    message(STATUS "Building ${PROJECT_NAME} for ${ALLOY_BOARD}")
endif()

if(CMAKE_CROSSCOMPILING AND DEFINED FLASH_COMMAND)
    add_custom_target(flash-${PROJECT_NAME}
        COMMAND ${FLASH_COMMAND}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Flashing ${PROJECT_NAME} to ${ALLOY_BOARD}"
    )
endif()

# ── Feature guard message ─────────────────────────────────────────────────────
# ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE and ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE
# are set by alloy_configure_selected_device() before this sub-project is added.

if(NOT ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE)
    message(WARNING
        "${PROJECT_NAME}: v2.1 codegen artifacts missing — "
        "run alloy-codegen to populate alloy/device/st/stm32g0/stm32g071rb/. "
        "The fallback main (fast LED blink) will be compiled instead.")
elseif(NOT ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE)
    message(WARNING
        "${PROJECT_NAME}: rcc_enable.hpp missing — "
        "regenerate with an up-to-date alloy-codegen. "
        "The fallback main will be compiled instead.")
else()
    message(STATUS
        "${PROJECT_NAME}: v2.1 lite HAL build — GPIO+UART+SPI+I2C+TIM+RCC enabled.")
endif()
