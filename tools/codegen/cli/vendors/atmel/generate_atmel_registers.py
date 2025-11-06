#!/usr/bin/env python3
"""
Generate Register Headers for Atmel/Microchip MCUs from SVD

This script integrates generic register generators with Atmel vendor-specific logic.
It leverages the existing infrastructure used by ST and other vendors for maximum
code reuse and consistency.

Key Features:
- Reuses generic SVD parser (cli/parsers/generic_svd.py)
- Reuses generic generators (registers, enums, register_map)
- Vendor-agnostic: Same approach works for all Atmel families
- Automatic bug fixes: Array dimensions expanded correctly (ABCDSR[%s] → ABCDSR[2])

Usage:
    # Generate for all board MCUs
    python generate_atmel_registers.py

    # Generate for specific MCU
    python generate_atmel_registers.py --mcu ATSAME70Q21B

    # Generate for entire family
    python generate_atmel_registers.py --family same70
"""

from pathlib import Path
import sys
from typing import List, Dict, Optional

# Add codegen directory to path
# This file is at: tools/codegen/cli/vendors/atmel/generate_atmel_registers.py
# Need to go up 3 levels to reach tools/codegen/
CODEGEN_DIR = Path(__file__).resolve().parent.parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

# Import generic generators (vendor-agnostic, reusable)
from cli.generators.generate_registers import generate_for_device as gen_registers
from cli.generators.generate_enums import generate_for_device as gen_enums
from cli.generators.generate_register_map import generate_for_device as gen_map

# Import SVD discovery and parsing
from cli.parsers.svd_discovery import discover_all_svds
from cli.parsers.generic_svd import parse_svd

# Import configuration and utilities
from cli.core.config import BOARD_MCUS, normalize_vendor, normalize_name
from cli.core.logger import print_header, print_success, print_error, print_info, logger
from cli.core.progress import get_global_tracker
from cli.core.paths import get_mcu_output_dir, ensure_dir


# ============================================================================
# ATMEL FAMILY CONFIGURATIONS
# ============================================================================

# Prioritized MCU families for Atmel/Microchip
# This allows generating high-priority families first (board MCUs)
ATMEL_FAMILIES = {
    "SAME70": {
        "priority": 1,  # Highest priority (board MCU)
        "description": "ARM Cortex-M7 @ 300MHz, 2MB Flash, 384KB RAM",
        "variants": [
            # Q variants (144-pin LQFP) - Highest pin count
            "ATSAME70Q21B", "ATSAME70Q21", "ATSAME70Q20B", "ATSAME70Q20",
            "ATSAME70Q19B", "ATSAME70Q19",
            # N variants (100-pin LQFP)
            "ATSAME70N21B", "ATSAME70N21", "ATSAME70N20B", "ATSAME70N20",
            "ATSAME70N19B", "ATSAME70N19",
            # J variants (64-pin LQFP)
            "ATSAME70J21B", "ATSAME70J21", "ATSAME70J20B", "ATSAME70J20",
            "ATSAME70J19B", "ATSAME70J19",
        ]
    },
    "SAMV71": {
        "priority": 1,  # Same priority as SAME70 (similar architecture)
        "description": "ARM Cortex-M7 @ 300MHz with additional peripherals",
        "variants": [
            # Q variants (144-pin LQFP)
            "ATSAMV71Q21B", "ATSAMV71Q21", "ATSAMV71Q20B", "ATSAMV71Q20",
            "ATSAMV71Q19B", "ATSAMV71Q19",
            # N variants (100-pin LQFP)
            "ATSAMV71N21B", "ATSAMV71N21", "ATSAMV71N20B", "ATSAMV71N20",
            "ATSAMV71N19B", "ATSAMV71N19",
            # J variants (64-pin LQFP)
            "ATSAMV71J21B", "ATSAMV71J21", "ATSAMV71J20B", "ATSAMV71J20",
            "ATSAMV71J19B", "ATSAMV71J19",
        ]
    },
    "SAMD21": {
        "priority": 2,  # Medium priority
        "description": "ARM Cortex-M0+ @ 48MHz (Arduino Zero, Feather)",
        "variants": [
            "ATSAMD21J18A", "ATSAMD21G18A", "ATSAMD21E18A",
            # More variants can be added as needed
        ]
    },
    "SAMD20": {
        "priority": 3,
        "description": "ARM Cortex-M0+ @ 48MHz",
        "variants": []  # Populated on demand
    },
    "SAM3X": {
        "priority": 2,
        "description": "ARM Cortex-M3 @ 84MHz (Arduino Due)",
        "variants": ["ATSAM3X8E"]
    },
}


# ============================================================================
# CORE GENERATOR FUNCTIONS
# ============================================================================

def generate_all_from_svd(svd_path: Path, tracker=None) -> bool:
    """
    Generate all register-related files from a single SVD file.

    This function orchestrates the generation of:
    1. Register structures (registers/*.hpp)
    2. Bit field definitions (bitfields/*.hpp)
    3. Enumerations (enums.hpp)
    4. Complete register map (register_map.hpp)

    All generators are vendor-agnostic and reusable.

    Args:
        svd_path: Path to SVD file
        tracker: Optional progress tracker

    Returns:
        True if all generations succeeded
    """
    logger.info(f"Generating from SVD: {svd_path.name}")

    try:
        # 1. Generate register structures
        # This creates registers/*.hpp with full register definitions
        logger.debug("  → Generating register structures...")
        if not gen_registers(svd_path, tracker):
            logger.error("  ✗ Failed to generate register structures")
            return False

        # 2. Generate enumerations
        # This creates enums.hpp with type-safe enum classes for register fields
        logger.debug("  → Generating enumerations...")
        if not gen_enums(svd_path, tracker):
            logger.error("  ✗ Failed to generate enumerations")
            return False

        # 3. Generate complete register map
        # This creates register_map.hpp (single-include header)
        logger.debug("  → Generating register map...")
        if not gen_map(svd_path, tracker):
            logger.error("  ✗ Failed to generate register map")
            return False

        logger.info(f"  ✓ Successfully generated all files from {svd_path.name}")
        return True

    except Exception as e:
        logger.error(f"  ✗ Generation failed: {e}")
        import traceback
        traceback.print_exc()
        return False


def find_svd_for_mcu(mcu_name: str, all_svds: Dict) -> Optional[Path]:
    """
    Find SVD file for a given MCU name.

    Handles various naming conventions:
    - Exact match: ATSAME70Q21B
    - Without 'B' suffix: ATSAME70Q21
    - With 'xx' suffix: ATSAME70Q21Bxx
    - Normalized names

    Args:
        mcu_name: MCU name to search for
        all_svds: Dictionary of discovered SVD files

    Returns:
        Path to SVD file if found, None otherwise
    """
    # Try exact match first
    if mcu_name in all_svds:
        return all_svds[mcu_name].file_path

    # Try common variants
    variants = [
        mcu_name,
        f"{mcu_name}xx",
        mcu_name.rstrip('B'),  # Try without 'B' suffix
        f"{mcu_name.rstrip('B')}xx",
        normalize_name(mcu_name),
        f"{normalize_name(mcu_name)}xx",
    ]

    for variant in variants:
        if variant in all_svds:
            logger.debug(f"Found SVD for {mcu_name} as {variant}")
            return all_svds[variant].file_path

    return None


# ============================================================================
# FAMILY-SPECIFIC GENERATION
# ============================================================================

def generate_for_family(family_name: str, family_config: Dict, all_svds: Dict,
                       tracker=None) -> tuple[int, int]:
    """
    Generate register files for all MCUs in a family.

    Args:
        family_name: Family name (e.g., "SAME70")
        family_config: Family configuration dictionary
        all_svds: Dictionary of all discovered SVD files
        tracker: Optional progress tracker

    Returns:
        Tuple of (success_count, fail_count)
    """
    print_info(f"\n{'='*80}")
    print_info(f"📦 Family: {family_name}")
    print_info(f"    {family_config['description']}")
    print_info(f"{'='*80}")

    variants = family_config.get("variants", [])
    if not variants:
        print_info("  ⚠️  No variants configured for this family")
        return (0, 0)

    success_count = 0
    fail_count = 0

    for mcu_name in variants:
        # Find SVD file
        svd_path = find_svd_for_mcu(mcu_name, all_svds)
        if not svd_path:
            logger.debug(f"  ⚠️  No SVD found for {mcu_name}")
            fail_count += 1
            continue

        # Generate all files from SVD
        print_info(f"  🔧 {mcu_name}...")
        if generate_all_from_svd(svd_path, tracker):
            success_count += 1
        else:
            fail_count += 1

    return (success_count, fail_count)


def generate_for_board_mcus(all_svds: Dict, tracker=None) -> tuple[int, int]:
    """
    Generate register files for all board MCUs (highest priority).

    Board MCUs are MCUs that have physical development boards in the project.
    These are prioritized for generation.

    Args:
        all_svds: Dictionary of all discovered SVD files
        tracker: Optional progress tracker

    Returns:
        Tuple of (success_count, fail_count)
    """
    print_info(f"\n{'='*80}")
    print_info(f"📋 Generating for Board MCUs (Highest Priority)")
    print_info(f"{'='*80}")

    # Filter board MCUs that are Atmel/Microchip
    atmel_board_mcus = []
    for mcu in BOARD_MCUS:
        if any(mcu.upper().startswith(prefix) for prefix in ["ATSAM", "AT91"]):
            atmel_board_mcus.append(mcu)

    if not atmel_board_mcus:
        print_info("  ⚠️  No Atmel board MCUs found")
        return (0, 0)

    print_info(f"  Found {len(atmel_board_mcus)} Atmel board MCU(s)")

    success_count = 0
    fail_count = 0

    for mcu_name in atmel_board_mcus:
        svd_path = find_svd_for_mcu(mcu_name, all_svds)
        if not svd_path:
            logger.warning(f"  ⚠️  No SVD found for board MCU: {mcu_name}")
            fail_count += 1
            continue

        print_info(f"  🔧 {mcu_name} (board MCU)...")
        if generate_all_from_svd(svd_path, tracker):
            success_count += 1
        else:
            fail_count += 1

    return (success_count, fail_count)


# ============================================================================
# MAIN ENTRY POINT
# ============================================================================

def main():
    """Main entry point for Atmel register generation"""
    import argparse

    parser = argparse.ArgumentParser(
        description='Generate register headers for Atmel/Microchip MCUs from SVD',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate for all board MCUs (default)
  python generate_atmel_registers.py

  # Generate for specific MCU
  python generate_atmel_registers.py --mcu ATSAME70Q21B

  # Generate for entire family
  python generate_atmel_registers.py --family same70

  # Generate for all Atmel MCUs
  python generate_atmel_registers.py --all

  # Verbose output
  python generate_atmel_registers.py --verbose
        """
    )

    parser.add_argument('--mcu', help='Generate for specific MCU (e.g., ATSAME70Q21B)')
    parser.add_argument('--family', help='Generate for specific family (e.g., same70, samv71)')
    parser.add_argument('--all', action='store_true', help='Generate for all Atmel families')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    args = parser.parse_args()

    # Setup logging
    if args.verbose:
        logger.setLevel('DEBUG')

    # Print header
    print_header("Atmel/Microchip Register Generator")
    print_info("Using vendor-agnostic generators for maximum code reuse")
    print_info("")

    # Setup progress tracker
    tracker = get_global_tracker()
    if tracker:
        tracker.set_generator("atmel_registers")

    # Discover all SVD files
    print_info("🔍 Discovering SVD files...")
    all_svds = discover_all_svds()

    # Filter for Atmel/Microchip SVDs
    atmel_svds = {
        name: info for name, info in all_svds.items()
        if info.vendor and info.vendor.lower() in ["atmel", "microchip", "microchip technology"]
    }

    print_info(f"   Found {len(atmel_svds)} Atmel/Microchip SVD files")
    print_info("")

    total_success = 0
    total_fail = 0

    # CASE 1: Generate for specific MCU
    if args.mcu:
        mcu_name = args.mcu.upper()
        svd_path = find_svd_for_mcu(mcu_name, atmel_svds)

        if not svd_path:
            print_error(f"No SVD found for MCU: {mcu_name}")
            return 1

        print_info(f"Generating for {mcu_name}...")
        if generate_all_from_svd(svd_path, tracker):
            print_success(f"Successfully generated for {mcu_name}")
            return 0
        else:
            print_error(f"Failed to generate for {mcu_name}")
            return 1

    # CASE 2: Generate for specific family
    elif args.family:
        family_name = args.family.upper()
        if family_name not in ATMEL_FAMILIES:
            print_error(f"Unknown family: {family_name}")
            print_info(f"Available families: {', '.join(ATMEL_FAMILIES.keys())}")
            return 1

        family_config = ATMEL_FAMILIES[family_name]
        success, fail = generate_for_family(family_name, family_config, atmel_svds, tracker)
        total_success += success
        total_fail += fail

    # CASE 3: Generate for all families
    elif args.all:
        # Sort families by priority
        sorted_families = sorted(
            ATMEL_FAMILIES.items(),
            key=lambda x: x[1].get('priority', 999)
        )

        for family_name, family_config in sorted_families:
            success, fail = generate_for_family(family_name, family_config, atmel_svds, tracker)
            total_success += success
            total_fail += fail

    # CASE 4: Default - Generate for board MCUs only
    else:
        success, fail = generate_for_board_mcus(atmel_svds, tracker)
        total_success += success
        total_fail += fail

    # Print summary
    print_info(f"\n{'='*80}")
    print_info(f"📊 Generation Summary")
    print_info(f"{'='*80}")
    print_success(f"✅ Successfully generated: {total_success} MCU(s)")
    if total_fail > 0:
        print_error(f"⚠️  Failed: {total_fail} MCU(s)")
    print_info("")

    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
