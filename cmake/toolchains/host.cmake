# Alloy Toolchain: Host (Native)
# For building and testing on the development machine

set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

# Use native compilers
# CMAKE_C_COMPILER and CMAKE_CXX_COMPILER will use system defaults

# Alloy-specific configuration for host
set(ALLOY_PLATFORM "host" CACHE STRING "Target platform")
set(ALLOY_ARCH "native" CACHE STRING "Target architecture")

# Host-specific flags (none needed for native builds)
set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_CXX_FLAGS_INIT "")

message(STATUS "Toolchain: Host (native compilation)")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Processor: ${CMAKE_SYSTEM_PROCESSOR}")
