#!/bin/bash
# Test build and flash blink example on all supported boards
#
# This script builds (and optionally flashes) the blink example for all
# MicroCore supported boards to verify cross-platform compatibility.
#
# Usage:
#   ./scripts/test-all-boards.sh          # Build only
#   ./scripts/test-all-boards.sh flash    # Build and flash (interactive)

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get the project root directory (one level up from scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Flash mode
FLASH_MODE=${1:-"build-only"}

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                  MicroCore Board Testing Script                              ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Project root: $PROJECT_ROOT"
echo "Mode: $FLASH_MODE"
echo ""

# Board configurations: board_name:description:led_info
BOARDS=(
    "nucleo_f401re:STM32F401RE Cortex-M4 84MHz:Green LED (LD2)"
    "nucleo_f722ze:STM32F722ZE Cortex-M7 216MHz:Green LED (LD1)"
    "nucleo_g071rb:STM32G071RB Cortex-M0+ 64MHz:Green LED (LD4)"
    "nucleo_g0b1re:STM32G0B1RE Cortex-M0+ 64MHz:Green LED (LD4)"
    "same70_xplained:ATSAME70Q21 Cortex-M7 300MHz:LED0 (green)"
)

SUCCESS_COUNT=0
FAIL_COUNT=0
FLASH_SUCCESS=0
FLASH_FAIL=0
FAILED_BOARDS=()
declare -a BUILD_RESULTS

for board_config in "${BOARDS[@]}"; do
    IFS=':' read -r board description led_info <<< "$board_config"

    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Testing: ${board}${NC}"
    echo -e "${BLUE}Description: ${description}${NC}"
    echo -e "${BLUE}Expected LED: ${led_info}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""

    BUILD_DIR="$PROJECT_ROOT/build-${board}"

    # Clean previous build
    echo "🧹 Cleaning previous build..."
    rm -rf "$BUILD_DIR"

    # Configure CMake
    echo "⚙️  Configuring CMake for ${board}..."
    if cmake -B "$BUILD_DIR" \
          -DMICROCORE_BOARD="${board}" \
          -DMICROCORE_BUILD_TESTS=OFF \
          -DCMAKE_TOOLCHAIN_FILE="${PROJECT_ROOT}/cmake/toolchains/arm-none-eabi.cmake" \
          -DCMAKE_BUILD_TYPE=Release \
          > "${BUILD_DIR}_config.log" 2>&1; then
        echo -e "${GREEN}✓ Configuration successful${NC}"
    else
        echo -e "${RED}✗ Configuration failed${NC}"
        echo "Last 20 lines of ${BUILD_DIR}_config.log:"
        tail -20 "${BUILD_DIR}_config.log"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_BOARDS+=("${board} (config)")
        BUILD_RESULTS+=("${board}: ${RED}✗ CONFIG FAILED${NC}")
        echo ""
        continue
    fi

    # Build the example
    echo "🔨 Building blink example..."
    if cmake --build "$BUILD_DIR" --target blink -j4 > "${BUILD_DIR}_build.log" 2>&1; then
        echo -e "${GREEN}✓ Build successful${NC}"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))

        # Display firmware size
        ELF_FILE="${BUILD_DIR}/examples/blink/blink.elf"
        if [ -f "$ELF_FILE" ]; then
            echo ""
            echo "📊 Binary Size:"
            if command -v arm-none-eabi-size &> /dev/null; then
                SIZE_OUTPUT=$(arm-none-eabi-size "$ELF_FILE")
                echo "$SIZE_OUTPUT"

                # Extract sizes
                FLASH_SIZE=$(echo "$SIZE_OUTPUT" | tail -1 | awk '{print $1 + $2}')
                RAM_SIZE=$(echo "$SIZE_OUTPUT" | tail -1 | awk '{print $2 + $3}')

                BUILD_RESULTS+=("${board}: ${GREEN}✓ BUILD OK${NC} (Flash: ${FLASH_SIZE}B, RAM: ${RAM_SIZE}B)")
            else
                BUILD_RESULTS+=("${board}: ${GREEN}✓ BUILD OK${NC}")
            fi
        else
            BUILD_RESULTS+=("${board}: ${GREEN}✓ BUILD OK${NC} (elf not found)")
        fi
    else
        echo -e "${RED}✗ Build failed${NC}"
        echo "Last 30 lines of ${BUILD_DIR}_build.log:"
        tail -30 "${BUILD_DIR}_build.log"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_BOARDS+=("${board} (build)")
        BUILD_RESULTS+=("${board}: ${RED}✗ BUILD FAILED${NC}")
        echo ""
        continue
    fi

    # Flash (if requested)
    if [ "$FLASH_MODE" == "flash" ]; then
        echo ""
        echo "📥 Ready to flash ${board}"
        echo "   Make sure the board is connected via ST-Link"
        echo "   Expected behavior: ${led_info} will blink at 1Hz"
        echo ""
        read -p "   Press ENTER to flash, or Ctrl+C to skip... " || true

        if cmake --build "$BUILD_DIR" --target flash > "${BUILD_DIR}_flash.log" 2>&1; then
            echo -e "${GREEN}✓ Flash successful!${NC}"
            echo -e "${YELLOW}   → Check if ${led_info} is blinking${NC}"
            FLASH_SUCCESS=$((FLASH_SUCCESS + 1))
        else
            echo -e "${RED}✗ Flash failed${NC}"
            echo "Last 20 lines of ${BUILD_DIR}_flash.log:"
            tail -20 "${BUILD_DIR}_flash.log"
            FLASH_FAIL=$((FLASH_FAIL + 1))
        fi
    fi

    echo ""
done

# Print summary
echo -e "${BLUE}╔══════════════════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                           TEST SUMMARY                                       ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "${YELLOW}Build Results:${NC}"
for result in "${BUILD_RESULTS[@]}"; do
    echo -e "  ${result}"
done
echo ""

echo -e "${YELLOW}Statistics:${NC}"
echo "  Total boards: ${#BOARDS[@]}"
echo -e "  Successful builds: ${GREEN}${SUCCESS_COUNT}${NC}"
if [ $FAIL_COUNT -gt 0 ]; then
    echo -e "  Failed builds: ${RED}${FAIL_COUNT}${NC}"
fi

if [ "$FLASH_MODE" == "flash" ]; then
    echo -e "  Successful flashes: ${GREEN}${FLASH_SUCCESS}${NC}"
    if [ $FLASH_FAIL -gt 0 ]; then
        echo -e "  Failed flashes: ${RED}${FLASH_FAIL}${NC}"
    fi
fi
echo ""

if [ $FAIL_COUNT -gt 0 ]; then
    echo -e "${RED}Failed boards:${NC}"
    for failed in "${FAILED_BOARDS[@]}"; do
        echo -e "  ${RED}✗ ${failed}${NC}"
    done
    echo ""
    echo -e "${RED}⚠️  Some builds failed!${NC}"
    exit 1
else
    echo -e "${GREEN}✅ All ${SUCCESS_COUNT} boards built successfully!${NC}"

    if [ "$FLASH_MODE" == "flash" ] && [ $FLASH_FAIL -gt 0 ]; then
        echo -e "${YELLOW}⚠️  Some flashes failed (but builds OK)${NC}"
        exit 2
    fi

    exit 0
fi
