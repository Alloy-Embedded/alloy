#!/bin/bash
# Build Script for ESP32 (Auto-Setup)
#
# Usage:
#   ./build-esp32.sh              # Build all ESP32 examples
#   ./build-esp32.sh rtos_blink   # Build specific example

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Alloy ESP32 Build Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Determine project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Check if ESP-IDF is in environment
if [ -z "$IDF_PATH" ]; then
    echo -e "${YELLOW}ESP-IDF not in environment.${NC}"
    echo ""

    # Priority 1: Project's own ESP-IDF (external/esp-idf)
    if [ -f "${PROJECT_ROOT}/external/esp-idf/export.sh" ]; then
        echo -e "${GREEN}Found project ESP-IDF at external/esp-idf${NC}"
        echo -e "${YELLOW}Setting up environment...${NC}"
        export IDF_PATH="${PROJECT_ROOT}/external/esp-idf"
        source "$IDF_PATH/export.sh"
    # Priority 2: System ESP-IDF (~esp/esp-idf)
    elif [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        echo -e "${GREEN}Found system ESP-IDF at ~/esp/esp-idf${NC}"
        echo -e "${YELLOW}Setting up environment...${NC}"
        export IDF_PATH="$HOME/esp/esp-idf"
        source "$IDF_PATH/export.sh"
    else
        echo -e "${RED}Error: ESP-IDF not found!${NC}"
        echo ""
        echo "Quick Setup (Recommended):"
        echo -e "${GREEN}  ./scripts/setup_esp_idf.sh${NC}"
        echo ""
        echo "Or install manually:"
        echo "  mkdir -p ~/esp"
        echo "  cd ~/esp"
        echo "  git clone --recursive https://github.com/espressif/esp-idf.git"
        echo "  cd esp-idf"
        echo "  ./install.sh esp32"
        echo ""
        exit 1
    fi
fi

echo -e "${GREEN}✓ ESP-IDF environment ready${NC}"
echo -e "${GREEN}  Path: $IDF_PATH${NC}"
echo ""

# Find Xtensa toolchain
XTENSA_GCC=$(which xtensa-esp32-elf-gcc 2>/dev/null || echo "")

if [ -z "$XTENSA_GCC" ]; then
    echo -e "${RED}Error: Xtensa toolchain not found in PATH${NC}"
    echo ""
    echo "Make sure ESP-IDF is properly configured:"
    echo "  source ~/esp/esp-idf/export.sh"
    exit 1
fi

echo -e "${GREEN}✓ Xtensa toolchain found${NC}"
echo -e "${GREEN}  GCC: $XTENSA_GCC${NC}"
echo ""

# Clean previous build (skip if running in Docker with mounted volume)
echo -e "${YELLOW}[1/3] Cleaning previous build...${NC}"
if [ -f /.dockerenv ] || [ -n "$DOCKER_CONTAINER" ]; then
    echo -e "${YELLOW}  Running in Docker container - skipping build directory removal${NC}"
    echo -e "${YELLOW}  (Using incremental build instead)${NC}"
    # Only clean CMake cache files if they exist
    rm -f build/CMakeCache.txt 2>/dev/null || true
else
    # Not in Docker, safe to remove build directory
    rm -rf build
fi

# Configure with Xtensa toolchain
echo -e "${YELLOW}[2/3] Configuring CMake...${NC}"
cmake -B build \
    -DALLOY_BOARD=esp32_devkit \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake

echo ""
echo -e "${GREEN}✓ Configuration complete${NC}"
echo ""

# Build
TARGET="${1:-rtos_blink_esp32}"
echo -e "${YELLOW}[3/3] Building target: ${TARGET}...${NC}"
cmake --build build --target "$TARGET"

echo ""
echo -e "${GREEN}✓ Build complete!${NC}"
echo ""

# Show binary info
BINARY="build/examples/${TARGET}/${TARGET}.bin"
if [ -f "$BINARY" ]; then
    SIZE=$(ls -lh "$BINARY" | awk '{print $5}')
    echo -e "${GREEN}Binary created: ${BINARY} (${SIZE})${NC}"
    echo ""
    echo "To flash to ESP32:"
    echo "  esptool.py --chip esp32 --port /dev/cu.usbserial-XXXX --baud 921600 write_flash -z 0x1000 $BINARY"
    echo ""
    echo "Or use the helper script:"
    echo "  cd examples/${TARGET}"
    echo "  ./flash_esp32.sh --monitor"
else
    # Try without .bin extension
    ELF="build/examples/${TARGET}/${TARGET}"
    if [ -f "$ELF" ]; then
        echo -e "${YELLOW}ELF file created but .bin conversion failed${NC}"
        echo "Creating .bin manually..."

        OBJCOPY=$(find ~/.espressif/tools/xtensa-esp-elf -name "xtensa-esp32-elf-objcopy" 2>/dev/null | head -1)
        if [ -n "$OBJCOPY" ]; then
            "$OBJCOPY" -O binary "$ELF" "$BINARY"
            SIZE=$(ls -lh "$BINARY" | awk '{print $5}')
            echo -e "${GREEN}✓ Binary created: ${BINARY} (${SIZE})${NC}"
        else
            echo -e "${RED}Error: objcopy not found${NC}"
            echo "ELF file: $ELF"
        fi
    fi
fi

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Build successful! 🎉${NC}"
echo -e "${BLUE}========================================${NC}"
