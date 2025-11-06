# ARM None EABI Toolchain for CMake
# This file configures CMake to use the ARM GCC toolchain for cross-compilation

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Skip compiler checks (they fail for cross-compilation)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# xPack ARM GCC toolchain path (prefer over Homebrew)
set(XPACK_ARM_GCC_BASE "$ENV{HOME}/Library/xPacks/@xpack-dev-tools/arm-none-eabi-gcc/14.2.1-1.1.1/.content/bin")

# Use xPack toolchain if available, otherwise search PATH
if(EXISTS "${XPACK_ARM_GCC_BASE}/arm-none-eabi-gcc")
    set(ARM_CC "${XPACK_ARM_GCC_BASE}/arm-none-eabi-gcc")
    set(ARM_CXX "${XPACK_ARM_GCC_BASE}/arm-none-eabi-g++")
    set(ARM_OBJCOPY "${XPACK_ARM_GCC_BASE}/arm-none-eabi-objcopy")
    set(ARM_SIZE "${XPACK_ARM_GCC_BASE}/arm-none-eabi-size")
    set(ARM_OBJDUMP "${XPACK_ARM_GCC_BASE}/arm-none-eabi-objdump")
    message(STATUS "Using xPack ARM GCC toolchain")
else()
    # Fallback to searching PATH
    find_program(ARM_CC arm-none-eabi-gcc)
    find_program(ARM_CXX arm-none-eabi-g++)
    find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
    find_program(ARM_SIZE arm-none-eabi-size)
    find_program(ARM_OBJDUMP arm-none-eabi-objdump)
    message(STATUS "Using system ARM GCC toolchain")
endif()

if(NOT ARM_CC)
    message(FATAL_ERROR "arm-none-eabi-gcc not found! Please install ARM GCC toolchain.")
endif()

# Set compilers
set(CMAKE_C_COMPILER ${ARM_CC})
set(CMAKE_CXX_COMPILER ${ARM_CXX})
set(CMAKE_ASM_COMPILER ${ARM_CC})
set(CMAKE_OBJCOPY ${ARM_OBJCOPY})
set(CMAKE_SIZE ${ARM_SIZE})
set(CMAKE_OBJDUMP ${ARM_OBJDUMP})

# Set find root path
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

message(STATUS "ARM Toolchain configured:")
message(STATUS "  CC:      ${CMAKE_C_COMPILER}")
message(STATUS "  CXX:     ${CMAKE_CXX_COMPILER}")
message(STATUS "  OBJCOPY: ${CMAKE_OBJCOPY}")
message(STATUS "  SIZE:    ${CMAKE_SIZE}")
