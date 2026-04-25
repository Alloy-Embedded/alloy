cmake_minimum_required(VERSION 3.25)

# Cooperative coroutine scheduler example: blink + heartbeat counter.
# See main.cpp for the design rationale; this file mirrors examples/blink
# for board wiring with one extra cosmetic difference -- tasks and the
# scheduler require C++23 (concepts + coroutine niceties).
project(tasks_blink_uart
    VERSION 1.0.0
    DESCRIPTION "Cooperative coroutine scheduler demo (blink + heartbeat)"
    LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 23)
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
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

if(DEFINED ALLOY_HAL_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ALLOY_HAL_LIBRARY})
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
