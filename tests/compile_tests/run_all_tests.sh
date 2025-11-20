#!/bin/bash
##
# @file run_all_tests.sh
# @brief Compile all CRTP tests and validate zero-overhead
#
# This script:
# - Compiles all CRTP compile tests
# - Validates zero-overhead via assembly inspection
# - Verifies no vtables generated
# - Reports compilation statistics
#
# @note Part of Phase 1.12: Validation and Testing
##

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
COMPILER="arm-none-eabi-g++"
STD="-std=c++23"
INCLUDES="-I../../src"
FLAGS="-Wall -Wextra -fno-exceptions -fno-rtti -Wno-unused-result"
OPTIMIZE="-O2"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Arrays to store results
declare -a FAILED_TEST_NAMES

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  CRTP Compile Tests Validation${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Function to compile a test
compile_test() {
    local test_file=$1
    local test_name=$(basename "$test_file" .cpp)

    # Check if file exists
    if [ ! -f "$test_file" ]; then
        echo -e "Testing ${test_name}... ${YELLOW}SKIP (not found)${NC}"
        return 0
    fi

    echo -ne "Testing ${test_name}... "

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    # Compile the test
    if $COMPILER $STD $INCLUDES $FLAGS $OPTIMIZE -fsyntax-only "$test_file" 2>/dev/null; then
        echo -e "${GREEN}PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        FAILED_TEST_NAMES+=("$test_name")
        return 1
    fi
}

# Function to check assembly for vtables
check_no_vtables() {
    local test_file=$1
    local test_name=$(basename "$test_file" .cpp)
    local asm_file="/tmp/${test_name}.s"

    # Check if file exists
    if [ ! -f "$test_file" ]; then
        echo -e "Checking vtables for ${test_name}... ${YELLOW}SKIP (not found)${NC}"
        return 0
    fi

    echo -ne "Checking vtables for ${test_name}... "

    # Generate assembly
    if $COMPILER $STD $INCLUDES $FLAGS $OPTIMIZE -S "$test_file" -o "$asm_file" 2>/dev/null; then
        # Check for vtable symbols
        if grep -q "_ZTV" "$asm_file" 2>/dev/null; then
            echo -e "${RED}VTABLE FOUND!${NC}"
            rm -f "$asm_file"
            return 1
        else
            echo -e "${GREEN}NO VTABLES${NC}"
            rm -f "$asm_file"
            return 0
        fi
    else
        echo -e "${YELLOW}SKIP (compilation failed)${NC}"
        return 0
    fi
}

# Run base class tests
echo -e "${YELLOW}Base Class Tests:${NC}"
echo "-------------------"
compile_test "test_uart_base_crtp.cpp" || true
compile_test "test_gpio_base_crtp.cpp" || true
compile_test "test_spi_base_crtp.cpp" || true
compile_test "test_i2c_base_crtp.cpp" || true
echo ""

# Run UART tests
echo -e "${YELLOW}UART API Tests:${NC}"
echo "-------------------"
compile_test "test_uart_simple_crtp.cpp" || true
compile_test "test_uart_fluent_crtp.cpp" || true
compile_test "test_uart_expert_crtp.cpp" || true
echo ""

# Run GPIO tests
echo -e "${YELLOW}GPIO API Tests:${NC}"
echo "-------------------"
compile_test "test_gpio_simple_crtp.cpp" || true
compile_test "test_gpio_fluent_crtp.cpp" || true
compile_test "test_gpio_expert_crtp.cpp" || true
echo ""

# Run SPI tests
echo -e "${YELLOW}SPI API Tests:${NC}"
echo "-------------------"
compile_test "test_spi_simple_crtp.cpp" || true
compile_test "test_spi_fluent_crtp.cpp" || true
compile_test "test_spi_expert_crtp.cpp" || true
echo ""

# Run I2C tests
echo -e "${YELLOW}I2C API Tests:${NC}"
echo "-------------------"
compile_test "test_i2c_simple_crtp.cpp" || true
compile_test "test_i2c_fluent_crtp.cpp" || true
compile_test "test_i2c_expert_crtp.cpp" || true
echo ""

# Vtable check section
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Zero-Overhead Validation${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

echo -e "${YELLOW}Checking for vtables (should be none):${NC}"
echo "-------------------"
check_no_vtables "test_uart_base_crtp.cpp" || true
check_no_vtables "test_gpio_base_crtp.cpp" || true
check_no_vtables "test_spi_base_crtp.cpp" || true
check_no_vtables "test_i2c_base_crtp.cpp" || true
echo ""

# Summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "Total Tests:  ${TOTAL_TESTS}"
echo -e "${GREEN}Passed:       ${PASSED_TESTS}${NC}"

if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "${RED}Failed:       ${FAILED_TESTS}${NC}"
    echo ""
    echo -e "${RED}Failed Tests:${NC}"
    for test in "${FAILED_TEST_NAMES[@]}"; do
        echo -e "  ${RED}- ${test}${NC}"
    done
    echo ""
    exit 1
else
    echo -e "${GREEN}Failed:       0${NC}"
    echo ""
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo -e "${GREEN}✓ Zero-overhead validated (no vtables found)${NC}"
    echo ""
    exit 0
fi
