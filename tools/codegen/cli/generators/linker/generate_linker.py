#!/usr/bin/env python3
"""
Generate Linker Scripts for Cortex-M MCUs

This generator creates linker scripts for different MCU families by reading
metadata JSON files and applying Jinja2 templates.

Features:
- Template-based linker script generation
- MCU-specific memory layouts
- Support for multiple RAM regions (main RAM, CCM, etc.)
- Configurable heap and stack sizes
- C++ support (constructors/destructors)
- Exception handling support

Usage:
    python3 generate_linker.py --mcu same70q21 --output boards/atmel_same70_xpld/ATSAME70Q21.ld
    python3 generate_linker.py --mcu stm32f407vg --output boards/stm32f407vg/STM32F407VG.ld
"""

import sys
import json
import argparse
from pathlib import Path
from datetime import datetime
from typing import Dict, Any

# Add parent directories to path
SCRIPT_DIR = Path(__file__).parent
CODEGEN_DIR = SCRIPT_DIR.parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, logger

try:
    from jinja2 import Environment, FileSystemLoader, select_autoescape
except ImportError:
    print_error("Jinja2 not installed. Run: pip3 install jinja2")
    sys.exit(1)


def load_metadata(mcu: str) -> Dict[str, Any]:
    """
    Load linker metadata for a specific MCU.

    Args:
        mcu: MCU name (e.g., 'same70q21', 'stm32f407vg')

    Returns:
        Dictionary with metadata

    Raises:
        FileNotFoundError: If metadata file doesn't exist
        json.JSONDecodeError: If metadata file is invalid JSON
    """
    # Linker metadata is now in cli/generators/metadata/linker/
    metadata_dir = SCRIPT_DIR.parent / "metadata" / "linker"
    metadata_file = metadata_dir / f"{mcu.lower()}_linker.json"

    if not metadata_file.exists():
        raise FileNotFoundError(
            f"Metadata file not found: {metadata_file}\n"
            f"Available MCUs: {list_available_mcus()}"
        )

    print_info(f"Loading metadata from {metadata_file.name}...")

    with open(metadata_file, 'r') as f:
        metadata = json.load(f)

    # Validate required fields
    required_fields = [
        'family', 'vendor', 'mcu_name', 'board_name', 'core',
        'memory_regions', 'flash_region', 'ram_region', 'stack_region',
        'min_heap_size', 'min_stack_size'
    ]

    missing = [field for field in required_fields if field not in metadata]
    if missing:
        raise ValueError(f"Metadata missing required fields: {missing}")

    print_success(f"Loaded metadata for {metadata['mcu_name']}")
    print_info(f"  Board: {metadata['board_name']}")
    print_info(f"  Core: {metadata['core']}")
    print_info(f"  Memory regions: {len(metadata['memory_regions'])}")

    return metadata


def list_available_mcus() -> list:
    """
    List all available linker metadata files.

    Returns:
        List of MCU names
    """
    # Linker metadata is now in cli/generators/metadata/linker/
    metadata_dir = SCRIPT_DIR.parent / "metadata" / "linker"
    if not metadata_dir.exists():
        return []

    mcus = []
    for file in metadata_dir.glob("*_linker.json"):
        mcu_name = file.stem.replace('_linker', '')
        mcus.append(mcu_name)

    return sorted(mcus)


def setup_jinja_environment() -> Environment:
    """
    Setup Jinja2 environment with custom filters and functions.

    Returns:
        Configured Jinja2 Environment
    """
    template_dir = CODEGEN_DIR / "templates" / "linker"

    env = Environment(
        loader=FileSystemLoader(str(template_dir)),
        autoescape=select_autoescape([]),  # No autoescape for linker scripts
        trim_blocks=True,
        lstrip_blocks=True,
        keep_trailing_newline=True
    )

    # Add custom filters
    def format_hex(value: str) -> str:
        """Format hex value consistently"""
        if isinstance(value, str) and value.startswith('0x'):
            return value
        return f"0x{int(value):08X}"

    def hex_to_kb(value: str) -> int:
        """Convert hex string to KB"""
        if isinstance(value, str) and value.startswith('0x'):
            return int(value, 16) // 1024
        return int(value) // 1024

    env.filters['format_hex'] = format_hex
    env.filters['hex_to_kb'] = hex_to_kb

    return env


def generate_linker(mcu: str, output_path: Path = None, dry_run: bool = False) -> bool:
    """
    Generate linker script for an MCU.

    Args:
        mcu: MCU name (e.g., 'same70q21', 'stm32f407vg')
        output_path: Output file path (optional)
        dry_run: If True, don't write file, just validate

    Returns:
        True if successful
    """
    try:
        # Load metadata
        metadata = load_metadata(mcu)

        # Setup Jinja2
        env = setup_jinja_environment()
        template = env.get_template('cortex_m.ld.j2')

        # Prepare template context
        context = {
            'metadata': metadata,
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        }

        # Render template
        print_info("Rendering template...")
        generated_code = template.render(**context)

        # Validate generated code (basic check)
        if not generated_code or len(generated_code) < 500:
            print_error("Generated code seems too short, something went wrong")
            return False

        print_success(f"Generated {len(generated_code)} bytes of linker script")

        if dry_run:
            print_info("Dry run mode - not writing file")
            print_info("\n" + "="*80)
            print_info("Generated linker script preview:")
            print_info("="*80)
            for i, line in enumerate(generated_code.split('\n')[:50], 1):
                print(f"{i:3d}: {line}")
            print_info("="*80)
            return True

        # Determine output path
        if not output_path:
            output_path = Path(f"boards/{mcu}/{metadata['mcu_name']}.ld")

        # Ensure output directory exists
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Write file
        print_info(f"Writing to {output_path}...")
        output_path.write_text(generated_code)

        print_success(f"Successfully generated linker script for {metadata['mcu_name']}")
        print_info(f"  Output: {output_path}")
        print_info(f"  Size: {len(generated_code)} bytes")
        print_info(f"  Lines: {len(generated_code.split(chr(10)))}")

        return True

    except FileNotFoundError as e:
        print_error(f"File not found: {e}")
        return False
    except json.JSONDecodeError as e:
        print_error(f"Invalid JSON in metadata file: {e}")
        return False
    except Exception as e:
        print_error(f"Generation failed: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Generate linker scripts for Cortex-M MCUs',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate for SAME70Q21
  python3 generate_linker.py --mcu same70q21

  # Generate for STM32F407VG with custom output
  python3 generate_linker.py --mcu stm32f407vg --output custom/path/linker.ld

  # Dry run (preview only)
  python3 generate_linker.py --mcu same70q21 --dry-run

  # List available MCUs
  python3 generate_linker.py --list
        """
    )

    parser.add_argument(
        '--mcu',
        help='Target MCU (e.g., same70q21, stm32f407vg)'
    )

    parser.add_argument(
        '--output', '-o',
        type=Path,
        help='Output file path (default: boards/{mcu}/{MCU}.ld)'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Preview generated code without writing file'
    )

    parser.add_argument(
        '--list', '-l',
        action='store_true',
        help='List available MCUs'
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )

    args = parser.parse_args()

    # Configure logging
    if args.verbose:
        logger.setLevel('DEBUG')

    # Print header
    print_header("Linker Script Generator")

    # List MCUs if requested
    if args.list:
        mcus = list_available_mcus()
        if mcus:
            print_info("Available MCUs:")
            for mcu in mcus:
                print(f"  • {mcu}")
        else:
            print_error("No metadata files found")
        return 0

    # Require mcu argument
    if not args.mcu:
        print_error("--mcu argument required (or use --list to see options)")
        parser.print_help()
        return 1

    # Generate
    success = generate_linker(
        mcu=args.mcu,
        output_path=args.output,
        dry_run=args.dry_run
    )

    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
