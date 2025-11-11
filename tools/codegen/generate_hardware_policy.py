#!/usr/bin/env python3
"""
Hardware Policy Generator

Generates C++ hardware policy headers from JSON metadata using Jinja2 templates.
Implements the Policy-Based Design pattern for zero-overhead hardware abstraction.

Usage:
    python generate_hardware_policy.py --family same70 --peripheral uart
    python generate_hardware_policy.py --family stm32f4 --peripheral spi
    python generate_hardware_policy.py --all  # Generate all policies

Architecture:
    This generator implements the Policy-Based Design pattern as specified in
    openspec/changes/modernize-peripheral-architecture/ARCHITECTURE.md

    Generated policies:
    - All methods are static inline (zero overhead)
    - Direct register access
    - Mock hooks for testing
    - Auto-generated from JSON metadata

Author: Alloy Code Generator
Date: 2025-11-10
"""

import json
import os
import sys
import argparse
from pathlib import Path
from datetime import datetime
from jinja2 import Environment, FileSystemLoader, Template

# Paths
SCRIPT_DIR = Path(__file__).parent
METADATA_DIR = SCRIPT_DIR / "cli" / "generators" / "metadata" / "platform"
TEMPLATE_DIR = SCRIPT_DIR / "templates" / "platform"
OUTPUT_BASE_DIR = SCRIPT_DIR.parent.parent / "src" / "hal" / "vendors"

def load_metadata(family: str, peripheral: str) -> dict:
    """
    Load JSON metadata for a specific family and peripheral.

    Args:
        family: MCU family (e.g., 'same70', 'stm32f4')
        peripheral: Peripheral type (e.g., 'uart', 'spi', 'i2c')

    Returns:
        Dictionary containing metadata

    Raises:
        FileNotFoundError: If metadata file doesn't exist
        JSONDecodeError: If metadata is invalid JSON
    """
    metadata_file = METADATA_DIR / f"{family}_{peripheral}.json"

    if not metadata_file.exists():
        raise FileNotFoundError(f"Metadata not found: {metadata_file}")

    print(f"📖 Loading metadata: {metadata_file}")

    with open(metadata_file, 'r') as f:
        metadata = json.load(f)

    # Validate required fields
    required_fields = ['family', 'vendor', 'peripheral_name', 'register_type']
    missing = [f for f in required_fields if f not in metadata]
    if missing:
        raise ValueError(f"Missing required fields in metadata: {missing}")

    # Check if policy_methods exists
    if 'policy_methods' not in metadata:
        print(f"⚠️  Warning: No 'policy_methods' section in metadata")
        print(f"   This is required for Policy-Based Design pattern")
        return None

    return metadata


def generate_policy_header(metadata: dict, template_path: Path, output_dir: Path) -> Path:
    """
    Generate hardware policy header from metadata and template.

    Args:
        metadata: Loaded JSON metadata
        template_path: Path to Jinja2 template
        output_dir: Directory for generated file

    Returns:
        Path to generated file
    """
    family = metadata['family']
    vendor = metadata['vendor']
    peripheral = metadata['peripheral_name'].lower()

    # Setup Jinja2 environment
    env = Environment(
        loader=FileSystemLoader(TEMPLATE_DIR),
        trim_blocks=True,
        lstrip_blocks=True
    )

    # Load template
    template = env.get_template(template_path.name)

    # Prepare template context
    context = {
        'metadata': metadata,
        'family': family,
        'vendor': vendor,
        'peripheral_name': peripheral.upper(),
        'generation_date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        'policy_methods': metadata.get('policy_methods', {})
    }

    # Render template
    print(f"🔧 Rendering template: {template_path.name}")
    generated_code = template.render(**context)

    # Create output directory
    output_path = output_dir / vendor / family
    output_path.mkdir(parents=True, exist_ok=True)

    # Write generated file
    output_file = output_path / f"{peripheral}_hardware_policy.hpp"
    with open(output_file, 'w') as f:
        f.write(generated_code)

    print(f"✅ Generated: {output_file}")
    return output_file


def generate_all_policies(family: str):
    """
    Generate all available hardware policies for a family.

    Args:
        family: MCU family (e.g., 'same70')
    """
    print(f"\n🚀 Generating all policies for {family.upper()}\n")

    peripherals = []
    for metadata_file in METADATA_DIR.glob(f"{family}_*.json"):
        peripheral = metadata_file.stem.replace(f"{family}_", "")
        peripherals.append(peripheral)

    if not peripherals:
        print(f"❌ No metadata files found for family: {family}")
        return

    print(f"Found peripherals: {', '.join(peripherals)}\n")

    success_count = 0
    skip_count = 0
    error_count = 0

    for peripheral in peripherals:
        try:
            print(f"\n{'='*60}")
            print(f"Processing: {family.upper()} {peripheral.upper()}")
            print(f"{'='*60}")

            metadata = load_metadata(family, peripheral)

            if metadata is None:
                print(f"⏭️  Skipping {peripheral} (no policy_methods)")
                skip_count += 1
                continue

            template_path = TEMPLATE_DIR / f"{peripheral}_hardware_policy.hpp.j2"
            if not template_path.exists():
                print(f"⚠️  Template not found: {template_path}")
                print(f"   Using generic template")
                template_path = TEMPLATE_DIR / "uart_hardware_policy.hpp.j2"

            generate_policy_header(metadata, template_path, OUTPUT_BASE_DIR)
            success_count += 1

        except Exception as e:
            print(f"❌ Error generating {peripheral}: {e}")
            error_count += 1

    print(f"\n{'='*60}")
    print(f"📊 Summary:")
    print(f"   ✅ Generated: {success_count}")
    print(f"   ⏭️  Skipped:   {skip_count}")
    print(f"   ❌ Errors:    {error_count}")
    print(f"{'='*60}\n")


def main():
    """Main entry point for the generator."""
    parser = argparse.ArgumentParser(
        description='Generate hardware policy headers from JSON metadata',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate UART policy for SAME70
  python generate_hardware_policy.py --family same70 --peripheral uart

  # Generate SPI policy for STM32F4
  python generate_hardware_policy.py --family stm32f4 --peripheral spi

  # Generate all policies for SAME70
  python generate_hardware_policy.py --family same70 --all

  # Generate policies for all families
  python generate_hardware_policy.py --all-families
        """
    )

    parser.add_argument(
        '--family',
        help='MCU family (e.g., same70, stm32f4)',
        type=str
    )

    parser.add_argument(
        '--peripheral',
        help='Peripheral type (e.g., uart, spi, i2c)',
        type=str
    )

    parser.add_argument(
        '--all',
        help='Generate all available policies for the specified family',
        action='store_true'
    )

    parser.add_argument(
        '--all-families',
        help='Generate policies for all available families',
        action='store_true'
    )

    parser.add_argument(
        '--output',
        help='Output directory (default: src/hal/vendors)',
        type=Path,
        default=OUTPUT_BASE_DIR
    )

    args = parser.parse_args()

    print("""
╔════════════════════════════════════════════════════════════════╗
║        Hardware Policy Generator (Policy-Based Design)         ║
║                  Alloy HAL Code Generator                      ║
╚════════════════════════════════════════════════════════════════╝
    """)

    try:
        if args.all_families:
            # Generate for all families
            families = set()
            for metadata_file in METADATA_DIR.glob("*_*.json"):
                family = metadata_file.stem.split('_')[0]
                families.add(family)

            for family in sorted(families):
                generate_all_policies(family)

        elif args.all:
            # Generate all peripherals for one family
            if not args.family:
                print("❌ Error: --family required with --all")
                sys.exit(1)

            generate_all_policies(args.family)

        elif args.family and args.peripheral:
            # Generate single policy
            metadata = load_metadata(args.family, args.peripheral)

            if metadata is None:
                print(f"❌ Cannot generate policy without 'policy_methods' section")
                sys.exit(1)

            template_path = TEMPLATE_DIR / f"{args.peripheral}_hardware_policy.hpp.j2"
            if not template_path.exists():
                print(f"⚠️  Specific template not found, using UART template")
                template_path = TEMPLATE_DIR / "uart_hardware_policy.hpp.j2"

            output_file = generate_policy_header(metadata, template_path, args.output)
            print(f"\n✅ Successfully generated hardware policy!")
            print(f"   File: {output_file}")

        else:
            parser.print_help()
            sys.exit(1)

    except Exception as e:
        print(f"\n❌ Fatal error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

    print("\n✨ Done!\n")


if __name__ == '__main__':
    main()
