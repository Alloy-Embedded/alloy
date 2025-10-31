#!/bin/bash
# Test build all 5 board examples
#
# This script builds all blink examples for different boards to verify
# that the modern Board API works consistently across all platforms.

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the project root directory (one level up from scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Project root: $PROJECT_ROOT"
echo ""

# Board configurations: board_name:example_name:toolchain
BOARDS=(
    "bluepill:stm32f103:arm-none-eabi"
    "stm32f407vg:stm32f407:arm-none-eabi"
    "rp_pico:rp_pico:arm-none-eabi"
    "arduino_zero:arduino_zero:arm-none-eabi"
    "esp32_devkit:esp32:arm-none-eabi"
)

SUCCESS_COUNT=0
FAIL_COUNT=0
FAILED_BOARDS=()

for board_config in "${BOARDS[@]}"; do
    IFS=':' read -r board example toolchain <<< "$board_config"
    echo "================================"
    echo "Testing: $board (example: blink_$example)"
    echo "================================"

    BUILD_DIR="$PROJECT_ROOT/build/test_$board"

    # Clean previous build
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure CMake
    echo -e "${YELLOW}Configuring CMake for $board...${NC}"
    if cmake -DALLOY_BOARD=$board \
          -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/toolchains/$toolchain.cmake" \
          "$PROJECT_ROOT" 2>&1 | tee cmake_config.log; then
        echo -e "${GREEN}CMake configuration successful${NC}"
    else
        echo -e "${RED}CMake configuration failed for $board${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_BOARDS+=("$board (CMake config)")
        cd "$PROJECT_ROOT"
        continue
    fi

    # Build the example
    echo -e "${YELLOW}Building blink_$example...${NC}"
    if cmake --build . --target blink_$example 2>&1 | tee build.log; then
        echo -e "${GREEN}Build successful for $board${NC}"

        # Find and display firmware size
        if [ -f "examples/blink_$example/blink_$example.elf" ]; then
            echo -e "${YELLOW}Firmware size:${NC}"
            arm-none-eabi-size "examples/blink_$example/blink_$example.elf" || true
        fi

        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        echo -e "${RED}Build failed for $board${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_BOARDS+=("$board (build)")
        cd "$PROJECT_ROOT"
        continue
    fi

    cd "$PROJECT_ROOT"
    echo ""
done

echo ""
echo "================================"
echo "Build Test Summary"
echo "================================"
echo -e "Total boards tested: ${#BOARDS[@]}"
echo -e "${GREEN}Successful builds: $SUCCESS_COUNT${NC}"
echo -e "${RED}Failed builds: $FAIL_COUNT${NC}"

if [ $FAIL_COUNT -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed boards:${NC}"
    for failed in "${FAILED_BOARDS[@]}"; do
        echo -e "  ${RED}- $failed${NC}"
    done
    exit 1
else
    echo ""
    echo -e "${GREEN}All 5 boards built successfully!${NC}"
    exit 0
fi
