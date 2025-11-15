# ==============================================================================
# STM32F7 Platform Configuration
# ==============================================================================
#
# Platform: STM32F7 (ARM Cortex-M7)
# Vendor: STMicroelectronics
# Architecture: ARM Cortex-M4 with FPU
# Examples: STM32F701, STM32F707, STM32F711, STM32F729
#
# ==============================================================================

message(STATUS "Configuring platform: STM32F7 (ARM Cortex-M7)")

# ------------------------------------------------------------------------------
# Platform Sources
# ------------------------------------------------------------------------------

# Collect STM32F7-specific platform sources
file(GLOB_RECURSE ALLOY_PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors/st/stm32f7/*.cpp
)

# Platform headers are in vendors directory (consolidated architecture)
set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors/st/stm32f7)

message(STATUS "  Platform sources: ${ALLOY_PLATFORM_SOURCES}")
message(STATUS "  Platform headers: ${ALLOY_PLATFORM_DIR}")

# ------------------------------------------------------------------------------
# Compiler Flags for Cortex-M7
# ------------------------------------------------------------------------------

# CPU-specific flags for STM32F7 (Cortex-M4 with FPU)
set(CPU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
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
)

message(STATUS "  CPU: Cortex-M7")
message(STATUS "  FPU: Hard float (FPv4-SP-D16)")
