# alloy::net — hardware-agnostic network abstraction layer.
#
# Provides:
#   alloy::net          — INTERFACE target: concept headers (no lwIP required)
#   alloy::net_lwip     — STATIC target: TcpStream + TcpListener + LwipAdapter
#                         (requires lwIP; adds ALLOY_LWIP_AVAILABLE define)
#
# lwIP is fetched via FetchContent at a pinned commit.
# The alloy::net INTERFACE target is always available on all platforms.

# ---- Concept-only INTERFACE target ----------------------------------------

add_library(alloy-net INTERFACE)
add_library(alloy::net ALIAS alloy-net)

target_include_directories(alloy-net INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."   # repo root → drivers/, src/
)

target_link_libraries(alloy-net INTERFACE alloy::alloy alloy::modbus)
target_compile_features(alloy-net INTERFACE cxx_std_23)

# ---- lwIP integration (optional) -----------------------------------------

option(ALLOY_BUILD_NETWORK_LWIP "Build lwIP-based TcpStream/TcpListener/LwipAdapter" ON)

if(ALLOY_BUILD_NETWORK_LWIP)
    include(FetchContent)

    # lwIP 2.2.0 stable release (pinned SHA).
    FetchContent_Declare(
        lwip
        GIT_REPOSITORY https://github.com/lwip-tcpip/lwip.git
        GIT_TAG        STABLE-2_2_0_RELEASE
        GIT_SHALLOW    TRUE
    )

    FetchContent_GetProperties(lwip)
    if(NOT lwip_POPULATED)
        FetchContent_Populate(lwip)
    endif()

    # lwIP CMakeLists is not always present; use the source files directly.
    set(_lwip_src_dir "${lwip_SOURCE_DIR}/src")

    file(GLOB_RECURSE _lwip_sources
        "${_lwip_src_dir}/core/*.c"
        "${_lwip_src_dir}/core/ipv4/*.c"
        "${_lwip_src_dir}/api/*.c"
        "${_lwip_src_dir}/netif/ethernet.c"
        "${_lwip_src_dir}/netif/etharp.c"
    )

    add_library(alloy-net-lwip STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/lwip/tcp_stream.cpp"
        ${_lwip_sources}
    )
    add_library(alloy::net_lwip ALIAS alloy-net-lwip)

    target_include_directories(alloy-net-lwip PUBLIC
        "${_lwip_src_dir}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lwip"   # lwipopts.h lives here
        "${CMAKE_CURRENT_SOURCE_DIR}/../.."  # repo root
    )

    target_compile_definitions(alloy-net-lwip PUBLIC ALLOY_LWIP_AVAILABLE)
    target_link_libraries(alloy-net-lwip PUBLIC alloy::net)
    target_compile_features(alloy-net-lwip PRIVATE cxx_std_23)

    # Host loopback test (requires lwIP LOOPIF).
    if(ALLOY_BUILD_TESTS)
        add_executable(test_net_lwip_loopback
            "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/net/test_lwip_loopback.cpp"
        )
        target_link_libraries(test_net_lwip_loopback PRIVATE
            alloy::net_lwip
            Catch2::Catch2WithMain
        )
        target_compile_features(test_net_lwip_loopback PRIVATE cxx_std_23)
        include(Catch)
        catch_discover_tests(test_net_lwip_loopback)
    endif()
endif()
