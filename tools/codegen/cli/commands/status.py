"""
Status command for Alloy CLI

This command generates implementation status reports.
"""

import sys
from pathlib import Path

# Add parent directory to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, logger


def setup_parser(parser):
    """Setup the status command parser"""
    parser.add_argument(
        '--output',
        type=Path,
        default=Path('MCU_STATUS.md'),
        help='Output file path (default: MCU_STATUS.md)'
    )

    parser.add_argument(
        '--vendor',
        type=str,
        choices=['st', 'atmel', 'microchip', 'all'],
        default='all',
        help='Show status for specific vendor (default: all)'
    )

    parser.add_argument(
        '--format',
        type=str,
        choices=['markdown', 'json', 'text'],
        default='markdown',
        help='Output format (default: markdown)'
    )


def execute(args):
    """Execute the status command"""
    print_header("Code Generation Status")

    try:
        # If format is markdown (default), generate the full MCU status report
        if args.format == 'markdown':
            from cli.generators.generate_mcu_status import generate_status_report
            return generate_status_report()
        
        # Otherwise, show simple terminal status
        from cli.core.config import BOARD_MCUS, HAL_VENDORS_DIR
        import json

        # Show board MCUs
        print(f"\n📋 Board MCUs: {len(BOARD_MCUS)}")
        for mcu in BOARD_MCUS:
            # Try to find the board name
            if mcu == "ATSAMD21G18A":
                print(f"  - {mcu} (arduino_zero)")
            elif mcu == "STM32F103":
                print(f"  - {mcu} (bluepill)")
            elif mcu == "ESP32":
                print(f"  - {mcu} (esp32_devkit)")
            elif mcu == "RP2040":
                print(f"  - {mcu} (rp_pico)")
            elif mcu == "ATSAME70Q21":
                print(f"  - {mcu} (same70_xpld)")
            elif mcu == "ATSAMV71Q21":
                print(f"  - {mcu} (samv71_xult)")
            elif mcu == "STM32F407":
                print(f"  - {mcu} (stm32f407vg)")
            elif mcu == "STM32F746":
                print(f"  - {mcu} (stm32f746disco)")
            else:
                print(f"  - {mcu}")

        # Check manifest
        manifest_path = HAL_VENDORS_DIR / ".generated_manifest.json"

        if manifest_path.exists():
            with open(manifest_path, 'r') as f:
                manifest = json.load(f)

            total_files = len(manifest.get('files', {}))
            last_updated = manifest.get('last_updated', 'Unknown')

            print(f"\n📝 Files generated: {total_files}")
            print(f"📅 Last generation: {last_updated}")
            print(f"\n💾 Manifest: {manifest_path}")
        else:
            print("\n⚠️  No manifest found. Run './codegen generate' to create it.")

        return 0

    except Exception as e:
        print_error(f"Status command failed: {e}")
        import traceback
        traceback.print_exc()
        return 1
