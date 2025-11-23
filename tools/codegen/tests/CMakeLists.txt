# Generated Code Validation Tests
#
# These tests validate that code generated from SVD files:
# - Compiles without errors or warnings (strict mode)
# - Has correct struct layouts matching datasheets
# - Follows project code style and conventions
# - Is type-safe and standards-compliant
#
# Tests run at build time to catch generation issues early.

cmake_minimum_required(VERSION 3.25)

project(MicroCoreCodegenValidation
    VERSION 1.0.0
    DESCRIPTION "Compile-time validation for MicroCore generated code"
    LANGUAGES CXX
)

# C++20 required for concepts and type traits
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strict warning flags - any warning is an error
set(STRICT_COMPILE_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Werror                # Treat warnings as errors
    -Wconversion           # Warn on implicit conversions
    -Wsign-conversion      # Warn on sign conversions
    -Wcast-align           # Warn on alignment issues
    -Wunused               # Warn on unused variables
    -Wshadow               # Warn on variable shadowing
)

# Include MicroCore headers (relative to project root)
set(MICROCORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
include_directories(
    "${MICROCORE_ROOT}/src"
    "${MICROCORE_ROOT}/include"
)

# ============================================================================
# STM32F4 Validation Test
# ============================================================================

add_executable(validate-stm32f4
    validate_stm32f4_generated.cpp
)

target_compile_options(validate-stm32f4 PRIVATE
    ${STRICT_COMPILE_FLAGS}
)

target_compile_definitions(validate-stm32f4 PRIVATE
    # Required defines for STM32F4 headers
    STM32F4
    STM32F401xE
)

# This test passes if it compiles (all static_asserts pass)
add_custom_target(test-stm32f4-validation
    COMMAND ${CMAKE_COMMAND} --build . --target validate-stm32f4
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "✓ Validating STM32F4 generated code..."
    VERBATIM
)

# ============================================================================
# SAME70 Validation Test
# ============================================================================

add_executable(validate-same70
    validate_same70_generated.cpp
)

target_compile_options(validate-same70 PRIVATE
    ${STRICT_COMPILE_FLAGS}
)

target_compile_definitions(validate-same70 PRIVATE
    # Required defines for SAME70 headers
    __SAME70Q21B__
    SAME70
)

# This test passes if it compiles (all static_asserts pass)
add_custom_target(test-same70-validation
    COMMAND ${CMAKE_COMMAND} --build . --target validate-same70
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "✓ Validating SAME70 generated code..."
    VERBATIM
)

# ============================================================================
# Master Validation Target
# ============================================================================

# Run all validation tests
add_custom_target(validate-generated-code
    DEPENDS validate-stm32f4 validate-same70
    COMMENT "=========================================="
    VERBATIM
)

add_custom_command(TARGET validate-generated-code POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "✓ All generated code validation passed!"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Validated:"
    COMMAND ${CMAKE_COMMAND} -E echo "  • STM32F4 register definitions"
    COMMAND ${CMAKE_COMMAND} -E echo "  • SAME70 register definitions"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "All struct layouts match datasheets ✓"
    COMMAND ${CMAKE_COMMAND} -E echo "All types are correct and volatile ✓"
    COMMAND ${CMAKE_COMMAND} -E echo "Zero-overhead principles preserved ✓"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    VERBATIM
)

# ============================================================================
# Code Style Validation (Optional - requires clang-format)
# ============================================================================

find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
    # Check if generated code follows style guide
    add_custom_target(validate-generated-style
        COMMAND ${CLANG_FORMAT} --dry-run --Werror
            "${MICROCORE_ROOT}/src/hal/vendors/st/stm32f4/generated/stm32f401/*.hpp"
            "${MICROCORE_ROOT}/src/hal/vendors/atmel/same70/generated/**/*.hpp"
        COMMENT "Validating generated code style..."
        VERBATIM
    )

    message(STATUS "clang-format found: ${CLANG_FORMAT}")
    message(STATUS "  Run 'make validate-generated-style' to check code style")
else()
    message(STATUS "clang-format not found (optional)")
    message(STATUS "  Install clang-format to validate generated code style")
endif()

# ============================================================================
# Usage Instructions
# ============================================================================

# Print usage instructions
message(STATUS "")
message(STATUS "==========================================")
message(STATUS "Generated Code Validation Tests")
message(STATUS "==========================================")
message(STATUS "")
message(STATUS "Build and run validation:")
message(STATUS "  cd tools/codegen/tests")
message(STATUS "  cmake -B build")
message(STATUS "  cmake --build build --target validate-generated-code")
message(STATUS "")
message(STATUS "Individual targets:")
message(STATUS "  validate-stm32f4   - Validate STM32F4 generated code")
message(STATUS "  validate-same70    - Validate SAME70 generated code")
message(STATUS "")
message(STATUS "==========================================")
message(STATUS "")
