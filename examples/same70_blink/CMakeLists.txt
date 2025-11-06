cmake_minimum_required(VERSION 3.20)

# NOTE: When configuring, use the toolchain file:
# cmake -DCMAKE_TOOLCHAIN_FILE=../arm-none-eabi-toolchain.cmake ..

# Project configuration
project(same70_blink
    VERSION 1.0.0
    LANGUAGES C CXX ASM
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# CPU and FPU settings for SAME70 (Cortex-M7 with double-precision FPU)
set(CPU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-d16  # Double-precision FPU
)

# Compiler flags
set(COMMON_FLAGS
    ${CPU_FLAGS}
    -Wall
    -Wextra
    -Wpedantic
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
    # Note: --specs=nano.specs removed (not available in all toolchains)
)

# Debug/Release configurations
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS
    "${CPU_FLAGS} \
    -T${CMAKE_SOURCE_DIR}/../../boards/atmel_same70_xpld/ATSAME70Q21.ld \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -nostartfiles \
    -Wl,-Map=${PROJECT_NAME}.map"
)

# Include directories
set(ALLOY_ROOT ${CMAKE_SOURCE_DIR}/../..)

include_directories(
    ${ALLOY_ROOT}/src
    ${ALLOY_ROOT}/boards/atmel_same70_xpld
)

# Source files
set(SOURCES
    main.cpp
    ${ALLOY_ROOT}/boards/atmel_same70_xpld/startup.cpp
)

# Create executable
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Apply compiler flags
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${COMMON_FLAGS})

# Generate additional output files
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Generating HEX and BIN files, showing size"
)

# Print build information
message(STATUS "====================================")
message(STATUS "ATSAME70 Xplained Blink Example")
message(STATUS "====================================")
message(STATUS "Board:    ATSAME70 Xplained")
message(STATUS "MCU:      ATSAME70Q21B")
message(STATUS "Core:     Cortex-M7 @ 300MHz max")
message(STATUS "FPU:      Double-precision (fpv5-d16)")
message(STATUS "Flash:    2MB")
message(STATUS "RAM:      384KB")
message(STATUS "====================================")
