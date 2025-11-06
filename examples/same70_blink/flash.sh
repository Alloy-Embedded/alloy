#!/bin/bash
# Flash script for ATSAME70 using OpenOCD

set -e

echo "======================================"
echo "ATSAME70 Flash Script"
echo "======================================"

# Check if build exists
if [ ! -f "build/same70_blink.elf" ]; then
    echo "Error: build/same70_blink.elf not found!"
    echo "Please run: mkdir -p build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../arm-none-eabi-toolchain.cmake .. && make"
    exit 1
fi

echo "Flashing device..."
openocd \
    -f interface/cmsis-dap.cfg \
    -f target/atsamv.cfg \
    -c "adapter speed 10000" \
    -c "init" \
    -c "targets" \
    -c "reset halt" \
    -c "flash write_image erase build/same70_blink.elf" \
    -c "verify_image build/same70_blink.elf" \
    -c "reset run" \
    -c "shutdown"

echo ""
echo "======================================"
echo "Flash successful!"
echo "======================================"
echo "LED should be blinking on PC8"
