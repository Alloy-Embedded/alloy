#!/bin/bash
# Quick flash script for ESP32 RTOS example
#
# Usage:
#   ./flash_esp32.sh              # Auto-detect port
#   ./flash_esp32.sh /dev/ttyUSB0 # Specific port
#   ./flash_esp32.sh --monitor    # Flash and monitor

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Alloy RTOS ESP32 Flash Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if esptool is installed
if ! command -v esptool.py &> /dev/null; then
    echo -e "${RED}Error: esptool.py not found${NC}"
    echo "Install with: pip install esptool"
    exit 1
fi

# Find ESP32 port
PORT=""
if [ $# -ge 1 ] && [ "$1" != "--monitor" ]; then
    PORT="$1"
else
    # Auto-detect port
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        PORT=$(ls /dev/cu.usbserial-* 2>/dev/null | head -1)
        if [ -z "$PORT" ]; then
            PORT=$(ls /dev/cu.SLAB_USBtoUART 2>/dev/null | head -1)
        fi
    else
        # Linux
        PORT=$(ls /dev/ttyUSB* 2>/dev/null | head -1)
    fi
fi

if [ -z "$PORT" ]; then
    echo -e "${RED}Error: No ESP32 found${NC}"
    echo "Connect your ESP32 and try again"
    echo ""
    echo "Available ports:"
    ls /dev/tty* /dev/cu.* 2>/dev/null | grep -E "(ttyUSB|usbserial|SLAB)" || echo "  None found"
    exit 1
fi

echo -e "${GREEN}✓ Found ESP32 at: ${PORT}${NC}"
echo ""

# Build project
echo -e "${YELLOW}[1/3] Building project...${NC}"
cd ../../..
if [ ! -d "build" ]; then
    echo "Configuring project..."
    cmake -B build \
        -DALLOY_BOARD=esp32_devkit \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake
fi

cmake --build build --target rtos_blink_esp32

BINARY="build/examples/rtos_blink_esp32/rtos_blink_esp32.bin"

if [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: Binary not found at $BINARY${NC}"
    exit 1
fi

SIZE=$(ls -lh "$BINARY" | awk '{print $5}')
echo -e "${GREEN}✓ Build complete (${SIZE})${NC}"
echo ""

# Flash to ESP32
echo -e "${YELLOW}[2/3] Flashing to ESP32...${NC}"
esptool.py --chip esp32 \
    --port "$PORT" \
    --baud 921600 \
    write_flash -z 0x1000 \
    "$BINARY"

echo -e "${GREEN}✓ Flash complete${NC}"
echo ""

# Monitor serial output
if [ "$1" == "--monitor" ] || [ "$2" == "--monitor" ]; then
    echo -e "${YELLOW}[3/3] Starting serial monitor (115200 baud)${NC}"
    echo -e "${BLUE}Press Ctrl+A then K to exit${NC}"
    echo ""
    sleep 1

    # Try screen first, fall back to minicom
    if command -v screen &> /dev/null; then
        screen "$PORT" 115200
    elif command -v minicom &> /dev/null; then
        minicom -D "$PORT" -b 115200
    else
        echo -e "${YELLOW}Install 'screen' or 'minicom' to monitor serial output${NC}"
        echo "Or use: screen $PORT 115200"
    fi
else
    echo -e "${GREEN}✓ Done!${NC}"
    echo ""
    echo "To monitor serial output:"
    echo "  screen $PORT 115200"
    echo ""
    echo "Or re-run with --monitor flag:"
    echo "  ./flash_esp32.sh --monitor"
fi
