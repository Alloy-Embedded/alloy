cmake_minimum_required(VERSION 3.25)

# modbus_tcp_slave — Modbus TCP slave on port 502 using GMAC + KSZ8081.
# Only builds for same70_xplained when both alloy::modbus and alloy::net_lwip
# are available.

if(NOT (ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld"))
    return()
endif()

if(NOT TARGET alloy::net_lwip)
    message(STATUS "modbus_tcp_slave: skipping (alloy::net_lwip not available)")
    return()
endif()

if(NOT TARGET alloy::modbus)
    message(STATUS "modbus_tcp_slave: skipping (alloy::modbus not available)")
    return()
endif()

add_executable(modbus_tcp_slave
    main.cpp
    ${ALLOY_ACTIVE_STARTUP_SOURCE}
)

target_link_libraries(modbus_tcp_slave PRIVATE
    alloy::alloy
    alloy::modbus
    alloy::net_lwip
)

target_include_directories(modbus_tcp_slave PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_definitions(modbus_tcp_slave PRIVATE
    ${ALLOY_TARGET_COMPILE_DEFINITIONS}
    SLAVE_ID=0x01
)

target_compile_features(modbus_tcp_slave PRIVATE cxx_std_23)

if(DEFINED LINKER_SCRIPT AND EXISTS "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}")
    target_link_options(modbus_tcp_slave PRIVATE
        -T "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}"
    )
endif()
