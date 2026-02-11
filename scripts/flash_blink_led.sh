#!/bin/bash
#
# Flash SAME70 generic blink_led example using BOSSA
#

set -e

PROJECT_ROOT="/Users/lgili/Documents/01 - Codes/01 - Github/corezero"
BIN_FILE="$PROJECT_ROOT/build-same70/examples/blink_led/blink_led.bin"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Flash Generic Blink LED (SAME70)${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if binary exists
if [ ! -f "$BIN_FILE" ]; then
    echo -e "${RED}✗ Error: Binary file not found!${NC}"
    echo "Expected: $BIN_FILE"
    echo ""
    echo "Please build first:"
    echo "  make same70-blink-generic-build"
    exit 1
fi

echo -e "${GREEN}✓${NC} Binary found: $(basename $BIN_FILE)"
echo -e "  Size: $(ls -lh $BIN_FILE | awk '{print $5}')"
echo ""

# Find USB device
USB_DEVICE=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)

if [ -z "$USB_DEVICE" ]; then
    echo -e "${RED}✗ No USB device found!${NC}"
    echo ""
    echo "Available devices:"
    ls -la /dev/cu.* | grep -E "(usb|debug)" || echo "  None"
    echo ""
    echo "Please check:"
    echo "  1. SAME70 Xplained is connected via USB (EDBG port)"
    echo "  2. Board is powered on"
    echo "  3. USB cable is working"
    exit 1
fi

echo -e "${GREEN}✓${NC} USB device found: $USB_DEVICE"
echo ""

echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}IMPORTANT: Put board in SAM-BA mode${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""
echo "To enter SAM-BA bootloader mode:"
echo ""
echo "  1. Press and HOLD the ERASE button"
echo "  2. Press and release the RESET button (while holding ERASE)"
echo "  3. Release the ERASE button"
echo ""
echo "The board is now in SAM-BA bootloader mode."
echo ""

read -p "Press ENTER after you've put the board in SAM-BA mode..."

# Wait for USB to re-enumerate
echo ""
echo "Waiting for SAM-BA bootloader..."
sleep 2

# Find device again
USB_DEVICE=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)

if [ -z "$USB_DEVICE" ]; then
    echo -e "${RED}✗ USB device not found after entering SAM-BA mode!${NC}"
    echo ""
    echo "Available devices:"
    ls -la /dev/cu.* | grep -E "(usb|debug)" || echo "  None"
    exit 1
fi

echo -e "${GREEN}✓${NC} SAM-BA device: $USB_DEVICE"
echo ""

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Flashing with BOSSA...${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Flash with BOSSA
bossac --port=$USB_DEVICE --erase --write --verify --boot=1 --reset "$BIN_FILE"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✅ Flash Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Expected behavior:"
echo "  - 5 fast blinks = SysTick initialized"
echo "  - 3 fast blinks = SysTick micros() working"
echo "  - Long pause (LED ON)"
echo "  - Slow blinks = delay_ms() working!"
echo ""
echo "If LED stays ON after pause, delay_ms() is hanging."
echo ""
