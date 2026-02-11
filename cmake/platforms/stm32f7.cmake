# ==============================================================================
# STM32F7 Platform Configuration
# ==============================================================================
#
# Platform: STM32F7 (ARM Cortex-M7)
# Vendor: STMicroelectronics
# Architecture: ARM Cortex-M7 with FPU and cache
# Examples: STM32F701, STM32F707, STM32F711, STM32F729
#
# ==============================================================================

message(STATUS "Configuring platform: STM32F7 (ARM Cortex-M7)")

# ------------------------------------------------------------------------------
# Platform Sources
# ------------------------------------------------------------------------------

# Collect STM32F7-specific platform sources
# EXCLUDE startup.cpp files - they are board-specific and added via STARTUP_SOURCE
file(GLOB_RECURSE MICROCORE_PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors/st/stm32f7/*.cpp
)

# Remove all startup*.cpp files from platform sources (board-specific)
list(FILTER MICROCORE_PLATFORM_SOURCES EXCLUDE REGEX ".*startup.*\\.cpp$")

# Platform headers are in vendors directory (consolidated architecture)
set(MICROCORE_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors/st/stm32f7)

message(STATUS "  Platform sources: ${MICROCORE_PLATFORM_SOURCES}")
message(STATUS "  Platform headers: ${MICROCORE_PLATFORM_DIR}")

# ------------------------------------------------------------------------------
# Compiler Flags for Cortex-M7
# ------------------------------------------------------------------------------

# CPU-specific flags for STM32F7 (Cortex-M7 with FPU)
set(CPU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-sp-d16
)

# Optimization flags
set(OPT_FLAGS
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
)

# Warning flags
set(WARN_FLAGS
    -Wall
    -Wextra
    -Wpedantic
)

# Apply flags globally
add_compile_options(${CPU_FLAGS} ${OPT_FLAGS} ${WARN_FLAGS})
add_link_options(${CPU_FLAGS})

# Linker flags for embedded
add_link_options(
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    --specs=nosys.specs
)

# ------------------------------------------------------------------------------
# Platform-Specific Definitions
# ------------------------------------------------------------------------------

add_compile_definitions(
    STM32F7
    ARM_MATH_CM7
    __FPU_PRESENT=1
    __FPU_USED=1
    __ICACHE_PRESENT=1
    __DCACHE_PRESENT=1
)

message(STATUS "  CPU: Cortex-M7")
message(STATUS "  FPU: Hard float (FPv5-SP-D16)")
