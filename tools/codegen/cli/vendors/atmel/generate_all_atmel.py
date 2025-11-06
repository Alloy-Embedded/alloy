#!/usr/bin/env python3
"""
Main generator for all Atmel/Microchip MCU families

This is the unified entry point for Atmel code generation. It orchestrates:
1. Register generation from SVD (automatic, vendor-agnostic)
2. Pin generation (vendor-specific, manual databases)

The order is important:
- Registers first (from SVD, fully automatic)
- Then pins (uses manual pin databases for alternate functions)
"""

import sys
from pathlib import Path

# Add codegen directory to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

try:
    from cli.vendors.atmel.generate_atmel_registers import main as main_registers
    from cli.vendors.atmel.generate_same70_pins import main_same70
    from cli.vendors.atmel.generate_samd21_pins import main as main_samd21
except ModuleNotFoundError:
    from generate_atmel_registers import main as main_registers
    from generate_same70_pins import main_same70
    from generate_samd21_pins import main as main_samd21


def main():
    """
    Generate code for all Atmel families.

    Generation pipeline:
    1. Registers from SVD (automatic) - New!
    2. Pins (manual databases) - Existing
    """
    print("\n" + "=" * 80)
    print("🏭 Alloy Atmel/Microchip Code Generator")
    print("=" * 80)
    print("Complete pipeline: Registers (SVD) + Pins (Manual)")
    print()

    # ========================================================================
    # PHASE 1: REGISTER GENERATION (Automatic from SVD)
    # ========================================================================
    print("\n" + "=" * 80)
    print("📦 PHASE 1: Register Generation from SVD")
    print("=" * 80)
    print()

    result = main_registers()
    if result != 0:
        print("\n⚠️  Register generation failed, but continuing with pins...")

    # ========================================================================
    # PHASE 2: PIN GENERATION (Manual databases)
    # ========================================================================
    print("\n" + "=" * 80)
    print("📦 PHASE 2: Pin Generation (SAME70/SAMV71)")
    print("=" * 80)
    print()

    result = main_same70()
    if result != 0:
        return result

    print()

    print("\n" + "=" * 80)
    print("📦 PHASE 2: Pin Generation (SAMD21)")
    print("=" * 80)
    print()

    result = main_samd21()
    if result != 0:
        return result

    # ========================================================================
    # SUMMARY
    # ========================================================================
    print("\n" + "=" * 80)
    print("✅ All Atmel/Microchip families generated successfully!")
    print("=" * 80)
    print()
    print("Generated:")
    print("  ✅ Registers (automatic from SVD)")
    print("     - Register structures (registers/*.hpp)")
    print("     - Bit fields (bitfields/*.hpp)")
    print("     - Enumerations (enums.hpp)")
    print("     - Register map (register_map.hpp)")
    print()
    print("  ✅ Pins (manual databases)")
    print("     - Pin definitions (pins.hpp)")
    print("     - Pin functions (pin_functions.hpp)")
    print("     - GPIO abstraction (gpio.hpp)")
    print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
