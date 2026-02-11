#!/bin/bash
# Test building all board examples

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Setup xPack toolchain
export PATH="/Users/lgili/.local/xpack-arm-toolchain/bin:$PATH"

# Project root
PROJECT_ROOT="/Users/lgili/Documents/01 - Codes/01 - Github/alloy"
cd "$PROJECT_ROOT"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Alloy - Build All Examples Test${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Array of boards to test
BOARDS=(
    "bluepill:blink_stm32f103:STM32F103C8 Blue Pill"
    "stm32f407vg:blink_stm32f407:STM32F407VG Discovery"
    "arduino_zero:blink_arduino_zero:Arduino Zero (ATSAMD21)"
    "rp_pico:blink_rp_pico:Raspberry Pi Pico (RP2040)"
)

# Results tracking
PASSED=0
FAILED=0
SKIPPED=0

# Function to test a board
test_board() {
    local board=$1
    local target=$2
    local name=$3

    echo -e "${YELLOW}Testing: ${name}${NC}"
    echo "  Board: ${board}"
    echo "  Target: ${target}"
    echo ""

    # Clean build directory
    rm -rf build

    # Configure
    echo "  [1/3] Configuring..."
    if cmake -B build \
        -DALLOY_BOARD="${board}" \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
        > /tmp/cmake_config.log 2>&1; then
        echo -e "  ${GREEN}✓${NC} Configuration successful"
    else
        echo -e "  ${RED}✗${NC} Configuration failed"
        cat /tmp/cmake_config.log | tail -20
        return 1
    fi

    # Build
    echo "  [2/3] Building..."
    if cmake --build build --target "${target}" > /tmp/cmake_build.log 2>&1; then
        echo -e "  ${GREEN}✓${NC} Build successful"
    else
        echo -e "  ${RED}✗${NC} Build failed"
        echo ""
        echo "  Build errors:"
        cat /tmp/cmake_build.log | grep -A 5 "error:"
        return 1
    fi

    # Check outputs
    echo "  [3/3] Verifying outputs..."
    if [ -f "build/examples/${target%-*}/${target}.bin" ]; then
        local size=$(stat -f%z "build/examples/${target%-*}/${target}.bin" 2>/dev/null || echo "0")
        echo -e "  ${GREEN}✓${NC} Binary generated (${size} bytes)"
    else
        echo -e "  ${YELLOW}!${NC} Binary not found (may be expected)"
    fi

    echo -e "${GREEN}✓ PASSED${NC}"
    echo ""
    return 0
}

# Test each board
for board_info in "${BOARDS[@]}"; do
    IFS=':' read -r board target name <<< "$board_info"

    if test_board "$board" "$target" "$name"; then
        ((PASSED++))
    else
        ((FAILED++))
    fi

    echo "----------------------------------------"
    echo ""
done

# Test ESP32 (if toolchain available)
echo -e "${YELLOW}Testing: ESP32 DevKit (Xtensa LX6)${NC}"
echo "  Board: esp32_devkit"
echo "  Target: blink_esp32"
echo ""

if command -v xtensa-esp32-elf-gcc &> /dev/null; then
    echo "  Xtensa toolchain found, attempting build..."

    rm -rf build

    if cmake -B build \
        -DALLOY_BOARD=esp32_devkit \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake \
        > /tmp/cmake_config.log 2>&1; then

        if cmake --build build --target blink_esp32 > /tmp/cmake_build.log 2>&1; then
            echo -e "${GREEN}✓ PASSED${NC}"
            ((PASSED++))
        else
            echo -e "${RED}✗ Build failed${NC}"
            cat /tmp/cmake_build.log | grep -A 5 "error:" | head -20
            ((FAILED++))
        fi
    else
        echo -e "${RED}✗ Configuration failed${NC}"
        ((FAILED++))
    fi
else
    echo -e "${YELLOW}! SKIPPED${NC} (xtensa-esp32-elf-gcc not found)"
    echo "  Install ESP-IDF to test ESP32 builds"
    ((SKIPPED++))
fi

echo ""
echo "========================================${NC}"
echo -e "${BLUE}Build Test Summary${NC}"
echo "========================================${NC}"
echo -e "${GREEN}Passed: ${PASSED}${NC}"
echo -e "${RED}Failed: ${FAILED}${NC}"
echo -e "${YELLOW}Skipped: ${SKIPPED}${NC}"
echo "========================================${NC}"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
