#!/bin/bash

# ==============================================================================
# Regenerate All GPIO/Pin Files - Complete Rebuild
# ==============================================================================
# This script regenerates all GPIO and pin files for all vendors after
# the GPIO improvements project.
#
# What it does:
# 1. Regenerates SAME70/SAMV71 pins (20 MCUs)
# 2. Regenerates SAMD21 pins (3 MCUs)
# 3. Cleans up old/unused HAL files
# 4. Reports summary
#
# Note: STM32, RP2040, and ESP32 don't have pin generators,
#       they were manually updated.
# ==============================================================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================================================"
echo "🚀 Regenerating All GPIO/Pin Files"
echo "================================================================================"
echo ""

# ==============================================================================
# 1. Regenerate SAME70/SAMV71 (20 MCUs)
# ==============================================================================

echo "📦 Step 1/4: Regenerating SAME70/SAMV71 pins..."
echo "   MCUs: 20 variants (SAME70 Q/N/J + SAMV71 Q/N/J)"
echo ""

PYTHONPATH=. python3 cli/vendors/atmel/generate_same70_pins.py
if [ $? -eq 0 ]; then
    echo "   ✅ SAME70/SAMV71 generated successfully"
else
    echo "   ❌ SAME70/SAMV71 generation failed"
    exit 1
fi

echo ""

# ==============================================================================
# 2. Regenerate SAMD21 (3 MCUs)
# ==============================================================================

echo "📦 Step 2/4: Regenerating SAMD21 pins..."
echo "   MCUs: 3 variants (G18A, E18A, J18A)"
echo ""

PYTHONPATH=. python3 cli/vendors/atmel/generate_samd21_pins.py
if [ $? -eq 0 ]; then
    echo "   ✅ SAMD21 generated successfully"
else
    echo "   ❌ SAMD21 generation failed"
    exit 1
fi

echo ""

# ==============================================================================
# 3. Clean up old/unused files
# ==============================================================================

echo "🧹 Step 3/4: Cleaning up old/unused files..."
echo ""

HAL_DIR="../../src/hal/vendors"

# List of files to remove (old external HAL templates that are now integrated)
FILES_TO_REMOVE=(
    # SAME70/SAMV71 - old pio_hal.hpp (now integrated in gpio.hpp)
    "$HAL_DIR/microchip/same70/pio_hal.hpp"
    "$HAL_DIR/microchip/samv71/pio_hal.hpp"

    # SAMD21 - old port_hal.hpp (now integrated in gpio.hpp)
    "$HAL_DIR/microchip/samd21/port_hal.hpp"

    # SAMD21 - old location (moved to atmel/)
    "$HAL_DIR/microchip/samd21/atsamd21g18a"
    "$HAL_DIR/microchip/samd21/atsamd21e18a"
    "$HAL_DIR/microchip/samd21/atsamd21j18a"

    # RP2040 - old external sio_hal.hpp (now integrated in gpio.hpp)
    "$HAL_DIR/raspberrypi/sio_hal.hpp"

    # STM32F4 - old gpio_hal.hpp backup
    "$HAL_DIR/st/stm32f4/gpio_hal_old.hpp.backup"
)

REMOVED_COUNT=0
SKIPPED_COUNT=0

for file in "${FILES_TO_REMOVE[@]}"; do
    if [ -e "$file" ]; then
        echo "   🗑️  Removing: $file"
        rm -rf "$file"
        REMOVED_COUNT=$((REMOVED_COUNT + 1))
    else
        echo "   ⏭️  Already removed: $file"
        SKIPPED_COUNT=$((SKIPPED_COUNT + 1))
    fi
done

echo ""
echo "   ✅ Cleanup complete: $REMOVED_COUNT files removed, $SKIPPED_COUNT already clean"
echo ""

# ==============================================================================
# 4. Summary Report
# ==============================================================================

echo "================================================================================"
echo "📊 Regeneration Summary"
echo "================================================================================"
echo ""
echo "✅ Generated Files:"
echo "   • SAME70/SAMV71: 20 MCUs × 3 files = 60 files"
echo "   • SAMD21:        3 MCUs × 3 files = 9 files"
echo "   • Total:         69 files generated"
echo ""
echo "✅ Updated Files (manual):"
echo "   • STM32F1:  gpio_hal.hpp (complete implementation)"
echo "   • STM32F4:  gpio_hal.hpp (compile-time optimization)"
echo "   • STM32F7:  gpio_hal.hpp (compile-time optimization)"
echo "   • RP2040:   gpio.hpp (integrated template)"
echo "   • ESP32:    Already optimized (no changes needed)"
echo ""
echo "🧹 Cleaned Files:"
echo "   • $REMOVED_COUNT old HAL files removed"
echo "   • $SKIPPED_COUNT files already clean"
echo ""
echo "📁 Output Locations:"
echo "   • SAME70/SAMV71: src/hal/vendors/microchip/{same70,samv71}/"
echo "   • SAMD21:        src/hal/vendors/atmel/samd21/"
echo "   • STM32:         src/hal/vendors/st/stm32{f1,f4,f7}/"
echo "   • RP2040:        src/hal/vendors/raspberrypi/rp2040/"
echo "   • ESP32:         src/hal/vendors/espressif/esp32/"
echo ""
echo "================================================================================"
echo "🎉 All GPIO files regenerated successfully!"
echo "================================================================================"
echo ""
echo "📖 Documentation:"
echo "   • tools/codegen/PIN_GPIO_ALL_COMPLETE.md"
echo "   • tools/codegen/PIN_GPIO_IMPROVEMENTS_COMPLETE.md (SAME70/SAMV71)"
echo "   • tools/codegen/PIN_GPIO_SAMD21_COMPLETE.md (SAMD21)"
echo "   • tools/codegen/PIN_GPIO_STM32_COMPLETE.md (STM32)"
echo ""
echo "✨ Next Steps:"
echo "   • Build your project to verify compilation"
echo "   • Test GPIO operations on target hardware"
echo "   • Enjoy type-safe, zero-overhead GPIO! 🚀"
echo ""
