# ==============================================================================
# STM32F4 Platform Configuration
# ==============================================================================
#
# Platform: STM32F4 (ARM Cortex-M4F)
# Vendor: STMicroelectronics
# Architecture: ARM Cortex-M4 with FPU
# Examples: STM32F401, STM32F407, STM32F411, STM32F429
#
# ==============================================================================

message(STATUS "Configuring platform: STM32F4 (ARM Cortex-M4F)")

# ------------------------------------------------------------------------------
# Platform Sources
# ------------------------------------------------------------------------------

# Collect STM32F4-specific platform sources
file(GLOB_RECURSE ALLOY_PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors/st/stm32f4/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/platform/st/stm32f4/*.cpp
)

# Platform headers are header-only, no .cpp files needed for platform layer
set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/platform/st/stm32f4)

message(STATUS "  Platform sources: ${ALLOY_PLATFORM_SOURCES}")
message(STATUS "  Platform headers: ${ALLOY_PLATFORM_DIR}")

# ------------------------------------------------------------------------------
# Compiler Flags for Cortex-M4F
# ------------------------------------------------------------------------------

# CPU-specific flags for STM32F4 (Cortex-M4 with FPU)
set(CPU_FLAGS
    -mcpu=cortex-m4
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
    STM32F4
    ARM_MATH_CM4
    __FPU_PRESENT=1
)

message(STATUS "  CPU: Cortex-M4F")
message(STATUS "  FPU: Hard float (FPv4-SP-D16)")
