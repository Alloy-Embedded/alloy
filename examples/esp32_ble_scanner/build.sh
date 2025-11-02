#!/bin/bash
# Quick build script for ESP32 BLE Scanner example
#
# Usage:
#   ./build.sh              # Clean build
#   ./build.sh flash        # Build and flash
#   ./build.sh monitor      # Build, flash, and monitor
#   ./build.sh menuconfig   # Open configuration menu

set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ESP32 BLE Scanner - Build Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if ESP-IDF is available
if [ -z "$IDF_PATH" ]; then
    echo -e "${YELLOW}ESP-IDF not in environment, loading...${NC}"
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        source "$HOME/esp/esp-idf/export.sh"
    else
        echo -e "${RED}Error: ESP-IDF not found!${NC}"
        echo "Please install ESP-IDF first:"
        echo "  https://docs.espressif.com/projects/esp-idf/en/latest/get-started/"
        exit 1
    fi
fi

echo -e "${GREEN}✓ ESP-IDF found at: $IDF_PATH${NC}"
echo ""

# Parse command
COMMAND=${1:-build}

case $COMMAND in
    build)
        echo -e "${BLUE}Building project...${NC}"
        idf.py build
        echo ""
        echo -e "${GREEN}✓ Build complete!${NC}"
        echo ""
        echo "To flash:"
        echo "  ./build.sh flash"
        ;;

    flash)
        echo -e "${BLUE}Building and flashing...${NC}"
        idf.py build flash
        echo ""
        echo -e "${GREEN}✓ Flash complete!${NC}"
        echo ""
        echo "To monitor:"
        echo "  idf.py -p PORT monitor"
        ;;

    monitor)
        echo -e "${BLUE}Building, flashing, and monitoring...${NC}"
        idf.py build flash monitor
        ;;

    menuconfig)
        echo -e "${BLUE}Opening configuration menu...${NC}"
        idf.py menuconfig
        ;;

    clean)
        echo -e "${BLUE}Cleaning build files...${NC}"
        rm -rf build
        echo -e "${GREEN}✓ Clean complete!${NC}"
        ;;

    *)
        echo "Usage: $0 [build|flash|monitor|menuconfig|clean]"
        echo ""
        echo "Commands:"
        echo "  build      - Build the project (default)"
        echo "  flash      - Build and flash to device"
        echo "  monitor    - Build, flash, and open serial monitor"
        echo "  menuconfig - Open configuration menu"
        echo "  clean      - Clean build files"
        exit 1
        ;;
esac
