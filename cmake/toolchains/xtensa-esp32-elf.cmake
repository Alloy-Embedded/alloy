# Alloy Toolchain: Xtensa ESP32 (bare-metal)
# For cross-compiling to ESP32 microcontrollers

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)

# Specify the cross compiler
set(CMAKE_C_COMPILER xtensa-esp32-elf-gcc)
set(CMAKE_CXX_COMPILER xtensa-esp32-elf-g++)
set(CMAKE_ASM_COMPILER xtensa-esp32-elf-gcc)
set(CMAKE_AR xtensa-esp32-elf-ar)
set(CMAKE_OBJCOPY xtensa-esp32-elf-objcopy)
set(CMAKE_OBJDUMP xtensa-esp32-elf-objdump)
set(CMAKE_SIZE xtensa-esp32-elf-size)
set(CMAKE_RANLIB xtensa-esp32-elf-ranlib)

# Don't try to run executables
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Alloy-specific configuration
set(ALLOY_PLATFORM "xtensa" CACHE STRING "Target platform")

# Common Xtensa ESP32 flags
# -mlongcalls: Use long calls for function calls (required for ESP32 memory layout)
# -mtext-section-literals: Place literals in text section (improves performance)
# -ffunction-sections -fdata-sections: Enable linker garbage collection
# -fno-exceptions -fno-rtti: Disable C++ features to reduce code size
set(CMAKE_C_FLAGS_INIT "-ffunction-sections -fdata-sections -mlongcalls")
set(CMAKE_CXX_FLAGS_INIT "-ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -mlongcalls")
set(CMAKE_ASM_FLAGS_INIT "-mlongcalls")

# Linker flags
# -Wl,--gc-sections: Enable garbage collection of unused sections
# -mlongcalls: Required for ESP32 memory layout
# -nostdlib: Don't link standard startup files
# -lc -lgcc: Link newlib C library and GCC runtime support
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections -mlongcalls -lc -lgcc")

# Build type specific flags
set(CMAKE_C_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG")

message(STATUS "Toolchain: Xtensa ESP32 (xtensa-esp32-elf-gcc)")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
