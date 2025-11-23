#!/bin/bash
#
# Flash SAME70 using BOSSA and set GPNVM1 boot bit
#

set -e

PROJECT_ROOT="/Users/lgili/Documents/01 - Codes/01 - Github/corezero"
BIN_FILE="$PROJECT_ROOT/build-same70/examples/same70_xplained_blink/same70_xplained_blink.bin"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}SAME70 Xplained - BOSSA Flash Tool${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if binary exists
if [ ! -f "$BIN_FILE" ]; then
    echo -e "${RED}✗ Error: Binary file not found!${NC}"
    echo "Expected: $BIN_FILE"
    echo ""
    echo "Please build first:"
    echo "  cd $PROJECT_ROOT"
    echo "  make same70-build"
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
echo "To enter SAM-BA bootloader mode, you need to:"
echo ""
echo "  1. Press and HOLD the ERASE button"
echo "  2. Press and release the RESET button (while holding ERASE)"
echo "  3. Release the ERASE button"
echo ""
echo "The board should now be in SAM-BA bootloader mode."
echo ""
echo -e "${BLUE}After doing this, the USB device may change.${NC}"
echo ""

read -p "Press ENTER after you've put the board in SAM-BA mode..."

# Wait a bit for USB to re-enumerate
echo ""
echo "Waiting for SAM-BA bootloader..."
sleep 2

# Find device again (it may have changed)
USB_DEVICE=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)

if [ -z "$USB_DEVICE" ]; then
    echo -e "${RED}✗ USB device not found after entering SAM-BA mode!${NC}"
    echo ""
    echo "Try again, or check if a new device appeared:"
    ls -la /dev/cu.* | grep usb || true
    exit 1
fi

echo -e "${GREEN}✓${NC} SAM-BA device: $USB_DEVICE"
echo ""

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Flashing with BOSSA...${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Use BOSSA to flash and set boot bit
# --port=<device>      : USB serial port
# --erase              : Erase entire flash before programming
# --write              : Write binary file
# --verify             : Verify after programming
# --boot=1             : Set GPNVM bit 1 (boot from Flash)
# --reset              : Reset MCU after programming

echo "Running: bossac --port=$USB_DEVICE --erase --write --verify --boot=1 --reset $BIN_FILE"
echo ""

bossac --port=$USB_DEVICE --erase --write --verify --boot=1 --reset "$BIN_FILE"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✅ Flash Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "The SAME70 should now:"
echo "  - Have your program in flash"
echo "  - Boot from flash (GPNVM1 set)"
echo "  - LED should be blinking!"
echo ""
echo "If LED is not blinking, press the RESET button."
echo ""
