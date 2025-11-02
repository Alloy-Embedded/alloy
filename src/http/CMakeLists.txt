# Alloy HTTP Component
#
# C++ abstraction layer for ESP-IDF HTTP server functionality

set(SRCS
    "server.cpp"
)

set(INCLUDE_DIRS
    "."
    "../core"
)

# Register component
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES
        esp_http_server
        esp_netif
)

# Set C++20 compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=c++20
    -Wall
    -Wextra
    -DESP_PLATFORM
)
