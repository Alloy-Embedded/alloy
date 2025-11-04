#!/usr/bin/env python3
"""
Main generator for all Atmel/Microchip MCU families
"""

import sys
from pathlib import Path

# Add codegen directory to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

try:
    from cli.vendors.atmel.generate_same70_pins import main_same70
    from cli.vendors.atmel.generate_samd21_pins import main as main_samd21
except ModuleNotFoundError:
    from generate_same70_pins import main_same70
    from generate_samd21_pins import main as main_samd21


def main():
    """Generate code for all Atmel families"""
    print("\n" + "=" * 80)
    print("🏭 Alloy Atmel/Microchip Code Generator")
    print("=" * 80)
    print()

    # Generate SAME70/SAMV71
    result = main_same70()
    if result != 0:
        return result

    print()

    # Generate SAMD21
    result = main_samd21()
    if result != 0:
        return result

    print("\n" + "=" * 80)
    print("✅ All Atmel/Microchip families generated successfully!")
    print("=" * 80)
    print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
