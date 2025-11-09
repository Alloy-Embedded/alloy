# Alloy Toolchain: ARM Cortex-M (bare-metal)
# For cross-compiling to ARM Cortex-M microcontrollers

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Prefer xPack toolchain if available
set(XPACK_TOOLCHAIN_PATH "$ENV{HOME}/.local/xpack-arm-toolchain/bin")
if(EXISTS "${XPACK_TOOLCHAIN_PATH}/arm-none-eabi-gcc")
    set(TOOLCHAIN_PREFIX "${XPACK_TOOLCHAIN_PATH}/")
    message(STATUS "Using xPack ARM toolchain from: ${XPACK_TOOLCHAIN_PATH}")
else()
    set(TOOLCHAIN_PREFIX "")
    message(STATUS "Using system ARM toolchain (install xPack for better compatibility)")
endif()

# Specify the cross compiler
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}arm-none-eabi-gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}arm-none-eabi-ar)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}arm-none-eabi-objdump)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}arm-none-eabi-size)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}arm-none-eabi-ranlib)

# Don't try to run executables
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Alloy-specific configuration
# Note: ALLOY_PLATFORM should be set by the board configuration, not the toolchain
# The toolchain is generic for all ARM Cortex-M targets

# Common ARM flags (will be specialized per board/MCU)
# -ffunction-sections -fdata-sections: Place each function/data in separate sections for better linker optimization
# -fno-exceptions -fno-rtti: Disable C++ exceptions and RTTI to reduce code size
# -fno-threadsafe-statics: Disable thread-safe static initialization (no threading in bare-metal)
set(CMAKE_C_FLAGS_INIT "-ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "-ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_ASM_FLAGS_INIT "")

# Linker flags
# -Wl,--gc-sections: Enable garbage collection of unused sections
#
# NOTE: xPack ARM toolchain v14.2.1+ has spec file conflicts with nosys.specs and nano.specs
# Workaround: We link libraries explicitly via target_link_libraries() in examples
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")

# Build type specific flags
set(CMAKE_C_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG")

message(STATUS "Toolchain: ARM Cortex-M (arm-none-eabi-gcc)")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
