# Alloy Toolchain: AVR (bare-metal)
# For cross-compiling to Microchip AVR family microcontrollers.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# Prefer avr-gcc from common install locations
find_program(AVR_GCC avr-gcc
    HINTS
        /usr/bin
        /usr/local/bin
        /opt/homebrew/bin
        "$ENV{HOME}/.local/avr-gcc/bin"
)

if(AVR_GCC)
    get_filename_component(TOOLCHAIN_BIN "${AVR_GCC}" DIRECTORY)
    set(TOOLCHAIN_PREFIX "${TOOLCHAIN_BIN}/")
    message(STATUS "Using AVR toolchain from: ${TOOLCHAIN_BIN}")
else()
    set(TOOLCHAIN_PREFIX "")
    message(WARNING "avr-gcc not found — install it (brew install avr-gcc or apt install gcc-avr)")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}avr-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}avr-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}avr-gcc)
set(CMAKE_AR           ${TOOLCHAIN_PREFIX}avr-ar)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}avr-objcopy)
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}avr-objdump)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}avr-size)
set(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}avr-ranlib)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# AVR128DA32 specific flags set by the board CMakeLists.
# The -mmcu flag must match the exact device string accepted by avr-gcc.
set(CMAKE_C_FLAGS_INIT   "-ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "-ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")

set(CMAKE_C_FLAGS_DEBUG_INIT   "-g -Og")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_C_FLAGS_RELEASE_INIT   "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Os -DNDEBUG")

message(STATUS "Toolchain: AVR (avr-gcc)")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
