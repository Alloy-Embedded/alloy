# CMake toolchain file for RL78 using GNURL78 (GCC for Renesas RL78)
#
# GNURL78 is the GCC-based toolchain for RL78 microcontrollers
# https://gcc-renesas.com/rl78/rl78-download-toolchains/
#
# Set RL78_TOOLCHAIN_PATH to the installation directory, e.g.:
#   -DRL78_TOOLCHAIN_PATH=/opt/gnurl78
#
# Or install to standard location and it will be auto-detected

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR rl78)

# Try to find GNURL78 toolchain
if(DEFINED ENV{RL78_TOOLCHAIN_PATH})
    set(RL78_TOOLCHAIN_PATH $ENV{RL78_TOOLCHAIN_PATH})
elseif(DEFINED RL78_TOOLCHAIN_PATH)
    # Use path provided via -DRL78_TOOLCHAIN_PATH
else()
    # Try standard installation paths
    if(EXISTS "/opt/gnurl78")
        set(RL78_TOOLCHAIN_PATH "/opt/gnurl78")
    elseif(EXISTS "/usr/local/gnurl78")
        set(RL78_TOOLCHAIN_PATH "/usr/local/gnurl78")
    elseif(EXISTS "C:/Program Files/GCC for Renesas RL78")
        set(RL78_TOOLCHAIN_PATH "C:/Program Files/GCC for Renesas RL78")
    endif()
endif()

if(RL78_TOOLCHAIN_PATH)
    set(TOOLCHAIN_PREFIX "${RL78_TOOLCHAIN_PATH}/bin/rl78-elf-")
    message(STATUS "Using GNURL78 toolchain from: ${RL78_TOOLCHAIN_PATH}")
else()
    # Try to find in PATH
    set(TOOLCHAIN_PREFIX "rl78-elf-")
    message(STATUS "RL78 toolchain not found in standard paths, trying PATH")
endif()

# Compiler executables
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}ranlib)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

# Compiler flags for RL78
# -mmul=g13: Use G13 multiplication instructions (common for RL78/G13)
# -mcpu=g13: Target RL78/G13 series (most common)
# -ffunction-sections, -fdata-sections: Enable dead code elimination
set(RL78_CPU_FLAGS "-mmul=g13 -mcpu=g13")
set(RL78_OPT_FLAGS "-ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS_INIT "${RL78_CPU_FLAGS} ${RL78_OPT_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${RL78_CPU_FLAGS} ${RL78_OPT_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${RL78_CPU_FLAGS}")

# Linker flags
# -Wl,--gc-sections: Remove unused sections
# -nostartfiles: Don't use standard startup files (we provide our own)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections -nostartfiles")

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and includes in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Skip compiler checks (cross-compiling)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

message(STATUS "Toolchain: RL78 (GNURL78)")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
