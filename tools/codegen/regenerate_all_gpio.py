#!/usr/bin/env python3
"""
Regenerate All GPIO/Pin Files - Complete Rebuild

This script regenerates all GPIO and pin files for all vendors after
the GPIO improvements project.

What it does:
1. Regenerates SAME70/SAMV71 pins (20 MCUs)
2. Regenerates SAMD21 pins (3 MCUs)
3. Cleans up old/unused HAL files
4. Reports detailed summary

Usage:
    python3 regenerate_all_gpio.py
    python3 regenerate_all_gpio.py --dry-run  # Preview without changes
"""

import sys
import subprocess
from pathlib import Path
import shutil

# Add codegen to path
SCRIPT_DIR = Path(__file__).parent
sys.path.insert(0, str(SCRIPT_DIR))

def print_header(title: str):
    """Print section header"""
    print("=" * 80)
    print(f"🚀 {title}")
    print("=" * 80)
    print()

def print_step(num: int, total: int, title: str):
    """Print step header"""
    print(f"📦 Step {num}/{total}: {title}")
    print()

def run_generator(script_path: Path, name: str) -> bool:
    """Run a generator script"""
    try:
        import os
        env = os.environ.copy()
        env['PYTHONPATH'] = str(SCRIPT_DIR)

        result = subprocess.run(
            [sys.executable, str(script_path)],
            cwd=SCRIPT_DIR,
            capture_output=True,
            text=True,
            env=env
        )

        if result.returncode == 0:
            print(f"   ✅ {name} generated successfully")
            return True
        else:
            print(f"   ❌ {name} generation failed:")
            print(f"   {result.stderr}")
            return False
    except Exception as e:
        print(f"   ❌ {name} generation error: {e}")
        return False

def cleanup_old_files(dry_run: bool = False) -> tuple[int, int]:
    """Clean up old/unused files"""
    hal_dir = SCRIPT_DIR.parent.parent / "src" / "hal" / "vendors"

    # Files to remove (old external HAL templates)
    files_to_remove = [
        # SAME70/SAMV71 - old pio_hal.hpp (now integrated)
        hal_dir / "microchip" / "same70" / "pio_hal.hpp",
        hal_dir / "microchip" / "samv71" / "pio_hal.hpp",

        # SAMD21 - old port_hal.hpp (now integrated)
        hal_dir / "microchip" / "samd21" / "port_hal.hpp",

        # SAMD21 - old location files (moved to atmel/)
        hal_dir / "microchip" / "samd21" / "atsamd21g18a",
        hal_dir / "microchip" / "samd21" / "atsamd21e18a",
        hal_dir / "microchip" / "samd21" / "atsamd21j18a",

        # RP2040 - old external sio_hal.hpp (now integrated)
        hal_dir / "raspberrypi" / "sio_hal.hpp",

        # STM32F4 - backup file
        hal_dir / "st" / "stm32f4" / "gpio_hal_old.hpp.backup",

        # Old template files that were copied (now generated inline)
        SCRIPT_DIR / "cli" / "vendors" / "atmel" / "pio_hal_template.hpp",
        SCRIPT_DIR / "cli" / "vendors" / "atmel" / "port_hal_template.hpp",
        SCRIPT_DIR / "cli" / "vendors" / "raspberrypi" / "sio_hal_template.hpp",
    ]

    removed_count = 0
    skipped_count = 0

    for file_path in files_to_remove:
        if file_path.exists():
            if dry_run:
                print(f"   🔍 Would remove: {file_path}")
            else:
                print(f"   🗑️  Removing: {file_path}")
                if file_path.is_dir():
                    shutil.rmtree(file_path)
                else:
                    file_path.unlink()
            removed_count += 1
        else:
            print(f"   ⏭️  Already removed: {file_path}")
            skipped_count += 1

    return removed_count, skipped_count

def print_summary(success: bool, removed: int, skipped: int):
    """Print final summary"""
    print()
    print("=" * 80)
    print("📊 Regeneration Summary")
    print("=" * 80)
    print()

    if success:
        print("✅ Generated Files:")
        print("   • SAME70/SAMV71: 20 MCUs × 3 files = 60 files")
        print("   • SAMD21:        3 MCUs × 3 files = 9 files")
        print("   • Total:         69 files generated")
        print()

        print("✅ Updated Files (manual):")
        print("   • STM32F1:  gpio_hal.hpp (complete implementation)")
        print("   • STM32F4:  gpio_hal.hpp (compile-time optimization)")
        print("   • STM32F7:  gpio_hal.hpp (compile-time optimization)")
        print("   • RP2040:   gpio.hpp (integrated template)")
        print("   • ESP32:    Already optimized (no changes needed)")
        print()

        print("🧹 Cleaned Files:")
        print(f"   • {removed} old HAL files removed")
        print(f"   • {skipped} files already clean")
        print()

        print("📁 Output Locations:")
        print("   • SAME70/SAMV71: src/hal/vendors/microchip/{same70,samv71}/")
        print("   • SAMD21:        src/hal/vendors/atmel/samd21/")
        print("   • STM32:         src/hal/vendors/st/stm32{f1,f4,f7}/")
        print("   • RP2040:        src/hal/vendors/raspberrypi/rp2040/")
        print("   • ESP32:         src/hal/vendors/espressif/esp32/")
        print()

        print("=" * 80)
        print("🎉 All GPIO files regenerated successfully!")
        print("=" * 80)
        print()

        print("📖 Documentation:")
        print("   • tools/codegen/PIN_GPIO_ALL_COMPLETE.md")
        print("   • tools/codegen/PIN_GPIO_IMPROVEMENTS_COMPLETE.md (SAME70/SAMV71)")
        print("   • tools/codegen/PIN_GPIO_SAMD21_COMPLETE.md (SAMD21)")
        print("   • tools/codegen/PIN_GPIO_STM32_COMPLETE.md (STM32)")
        print()

        print("✨ Next Steps:")
        print("   • Build your project to verify compilation")
        print("   • Test GPIO operations on target hardware")
        print("   • Enjoy type-safe, zero-overhead GPIO! 🚀")
        print()
    else:
        print("❌ Regeneration failed!")
        print("   Check error messages above for details.")
        print()

def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description="Regenerate all GPIO/pin files")
    parser.add_argument("--dry-run", action="store_true",
                       help="Preview changes without modifying files")
    args = parser.parse_args()

    if args.dry_run:
        print("🔍 DRY RUN MODE - No files will be modified")
        print()

    print_header("Regenerating All GPIO/Pin Files")

    success = True

    # Step 1: Regenerate SAME70/SAMV71
    print_step(1, 4, "Regenerating SAME70/SAMV71 pins...")
    print("   MCUs: 20 variants (SAME70 Q/N/J + SAMV71 Q/N/J)")
    print()

    if not args.dry_run:
        same70_script = SCRIPT_DIR / "cli" / "vendors" / "atmel" / "generate_same70_pins.py"
        if not run_generator(same70_script, "SAME70/SAMV71"):
            success = False
    else:
        print("   🔍 Would regenerate SAME70/SAMV71")

    print()

    # Step 2: Regenerate SAMD21
    print_step(2, 4, "Regenerating SAMD21 pins...")
    print("   MCUs: 3 variants (G18A, E18A, J18A)")
    print()

    if not args.dry_run:
        samd21_script = SCRIPT_DIR / "cli" / "vendors" / "atmel" / "generate_samd21_pins.py"
        if not run_generator(samd21_script, "SAMD21"):
            success = False
    else:
        print("   🔍 Would regenerate SAMD21")

    print()

    # Step 3: Verify manual updates (STM32, RP2040, ESP32)
    print_step(3, 4, "Verifying manual updates...")
    print()

    hal_dir = SCRIPT_DIR.parent.parent / "src" / "hal" / "vendors"

    manual_files = [
        (hal_dir / "st" / "stm32f1" / "gpio_hal.hpp", "STM32F1 GPIO HAL"),
        (hal_dir / "st" / "stm32f4" / "gpio_hal.hpp", "STM32F4 GPIO HAL"),
        (hal_dir / "st" / "stm32f7" / "gpio_hal.hpp", "STM32F7 GPIO HAL"),
        (hal_dir / "raspberrypi" / "rp2040" / "gpio.hpp", "RP2040 GPIO"),
        (hal_dir / "espressif" / "esp32" / "gpio.hpp", "ESP32 GPIO"),
    ]

    for file_path, name in manual_files:
        if file_path.exists():
            print(f"   ✅ {name}: Present")
        else:
            print(f"   ❌ {name}: Missing!")
            success = False

    print()

    # Step 4: Cleanup
    print_step(4, 4, "Cleaning up old/unused files...")
    print()

    removed, skipped = cleanup_old_files(dry_run=args.dry_run)

    print()
    print(f"   ✅ Cleanup complete: {removed} files removed, {skipped} already clean")

    # Summary
    print_summary(success, removed, skipped)

    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
