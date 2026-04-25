cmake_minimum_required(VERSION 3.25)

# ethernet_basic — link up GMAC + KSZ8081 on SAME70 Xplained, DHCP, ping reply.
# Only builds for same70_xplained.

if(NOT (ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld"))
    return()
endif()

if(NOT TARGET alloy::net_lwip)
    message(STATUS "ethernet_basic: skipping (alloy::net_lwip not available — "
                   "set ALLOY_BUILD_NETWORK_LWIP=ON)")
    return()
endif()

add_executable(ethernet_basic
    main.cpp
    ${ALLOY_ACTIVE_STARTUP_SOURCE}
)

target_link_libraries(ethernet_basic PRIVATE
    alloy::alloy
    alloy::net_lwip
)

target_include_directories(ethernet_basic PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_definitions(ethernet_basic PRIVATE
    ${ALLOY_TARGET_COMPILE_DEFINITIONS}
)

target_compile_features(ethernet_basic PRIVATE cxx_std_23)

if(DEFINED LINKER_SCRIPT AND EXISTS "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}")
    target_link_options(ethernet_basic PRIVATE
        -T "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}"
    )
endif()
