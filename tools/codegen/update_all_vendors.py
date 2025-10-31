#!/usr/bin/env python3
"""
Update All Vendors - Complete SVD to C++ Generation Pipeline

This script automates the complete process:
1. Scans all SVD files from cmsis-svd-data
2. Parses SVD files to generate JSON databases
3. Generates C++ peripheral code for all MCUs

Usage:
    python3 update_all_vendors.py --all                    # Process all vendors
    python3 update_all_vendors.py --vendor STMicro         # Process specific vendor
    python3 update_all_vendors.py --vendor STMicro --family stm32f1  # Specific family
"""

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Dict, List
import re

SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent.parent
SVD_DIR = SCRIPT_DIR / "upstream" / "cmsis-svd-data" / "data"
DATABASE_DIR = SCRIPT_DIR / "database" / "families"
OUTPUT_DIR = REPO_ROOT / "src" / "generated"

class Colors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    INFO = '\033[96m'
    BOLD = '\033[1m'
    ENDC = '\033[0m'

def print_success(msg: str):
    print(f"{Colors.OKGREEN}✓{Colors.ENDC} {msg}")

def print_info(msg: str):
    print(f"{Colors.INFO}→{Colors.ENDC} {msg}")

def print_warning(msg: str):
    print(f"{Colors.WARNING}⚠{Colors.ENDC} {msg}")

def print_error(msg: str):
    print(f"{Colors.FAIL}✗{Colors.ENDC} {msg}")

def print_header(msg: str):
    print(f"\n{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"{Colors.BOLD}{msg}{Colors.ENDC}")
    print(f"{Colors.BOLD}{'='*70}{Colors.ENDC}\n")


def get_vendor_mapping(vendor_dir: str) -> str:
    """Map vendor directory names to clean names"""
    mappings = {
        "STMicro": "st",
        "Nordic": "nordic",
        "NXP": "nxp",
        "Freescale": "nxp",  # Freescale is now NXP
        "Atmel": "atmel",
        "SiliconLabs": "silabs",
        "Toshiba": "toshiba",
        "Holtek": "holtek",
        "Nuvoton": "nuvoton",
        "Espressif": "espressif",
        "Cypress": "cypress",
        "GigaDevice": "gigadevice",
        "Renesas": "renesas",
        "Fujitsu": "fujitsu"
    }
    return mappings.get(vendor_dir, vendor_dir.lower())


def extract_family_from_name(mcu_name: str, vendor: str) -> str:
    """Extract family from MCU name based on vendor conventions"""
    mcu_lower = mcu_name.lower()

    # STM32 families
    if mcu_lower.startswith("stm32"):
        # STM32F103xx -> stm32f1, STM32F446RE -> stm32f4
        match = re.match(r'stm32([a-z]\d+)', mcu_lower)
        if match:
            return f"stm32{match.group(1)}"
        return "stm32"

    # Nordic nRF families
    elif mcu_lower.startswith("nrf"):
        # nRF52840 -> nrf52, nRF51822 -> nrf51
        match = re.match(r'nrf(\d+)', mcu_lower)
        if match:
            return f"nrf{match.group(1)}"
        return "nrf"

    # Atmel SAM families
    elif mcu_lower.startswith("at"):
        if "samd" in mcu_lower:
            return "samd"
        elif "same" in mcu_lower:
            return "same"
        elif "saml" in mcu_lower:
            return "saml"
        elif "sam9" in mcu_lower:
            return "sam9"
        elif "sama5" in mcu_lower:
            return "sama5"
        return "sam"

    # ESP32 families
    elif mcu_lower.startswith("esp"):
        # ESP32, ESP32-C3, ESP32-S3 -> esp32, esp32c3, esp32s3
        match = re.match(r'esp(\d+[a-z]?\d*)', mcu_lower.replace('-', ''))
        if match:
            return f"esp{match.group(1)}"
        return "esp32"

    # NXP Kinetis families
    elif mcu_lower.startswith("mk"):
        # MKL46Z4 -> mkl4, MK64FN1M0 -> mk6
        match = re.match(r'mk([a-z]?\d+)', mcu_lower)
        if match:
            return f"mk{match.group(1)}"
        return "kinetis"

    # Default: first word
    return mcu_lower.split('_')[0].split('-')[0][:10]


def find_svd_files(vendor_filter: str = None, family_filter: str = None) -> Dict[str, List[Path]]:
    """Find all SVD files organized by vendor"""

    if not SVD_DIR.exists():
        print_error(f"SVD directory not found: {SVD_DIR}")
        print_info("Run: git submodule update --init --recursive")
        sys.exit(1)

    vendors = {}

    for vendor_dir in sorted(SVD_DIR.iterdir()):
        if not vendor_dir.is_dir():
            continue

        vendor_name = vendor_dir.name

        # Skip if vendor filter specified and doesn't match
        if vendor_filter and vendor_name.lower() != vendor_filter.lower():
            continue

        # Find SVD files
        svd_files = list(vendor_dir.glob("*.svd"))

        if not svd_files:
            continue

        # Filter by family if specified
        if family_filter:
            filtered_files = []
            for svd_file in svd_files:
                mcu_name = svd_file.stem
                family = extract_family_from_name(mcu_name, vendor_name)
                if family_filter.lower() in family.lower():
                    filtered_files.append(svd_file)
            svd_files = filtered_files

        if svd_files:
            vendors[vendor_name] = svd_files

    return vendors


def parse_svd_file(svd_path: Path, output_path: Path) -> bool:
    """Parse a single SVD file to JSON database"""

    cmd = [
        sys.executable,
        str(SCRIPT_DIR / "svd_parser.py"),
        "--input", str(svd_path),
        "--output", str(output_path)
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print_error(f"Failed to parse {svd_path.name}")
        if result.stderr:
            print(result.stderr)
        return False

    return True


def process_vendor(vendor_name: str, svd_files: List[Path], family_filter: str = None) -> int:
    """Process all SVD files for a vendor"""

    print_header(f"Processing {vendor_name} ({len(svd_files)} SVD files)")

    vendor_clean = get_vendor_mapping(vendor_name)
    success_count = 0
    failed_files = []

    # Group files by family
    families = {}
    for svd_file in svd_files:
        mcu_name = svd_file.stem
        family = extract_family_from_name(mcu_name, vendor_name)

        if family not in families:
            families[family] = []
        families[family].append(svd_file)

    print_info(f"Found {len(families)} families: {', '.join(sorted(families.keys()))}")

    # Process each family
    for family, files in sorted(families.items()):
        if family_filter and family_filter.lower() not in family.lower():
            continue

        print_info(f"\n  Family: {family} ({len(files)} MCUs)")

        # Create database output path
        db_output = DATABASE_DIR / f"{vendor_clean}_{family}.json"

        # For families with multiple MCUs, we'll merge them into one database
        # For now, process the first representative MCU
        # TODO: In future, merge multiple MCUs into single family database

        representative_file = files[0]  # Take first file as representative
        print_info(f"    Parsing {representative_file.name}...")

        if parse_svd_file(representative_file, db_output):
            print_success(f"    Generated: {db_output.name}")
            success_count += 1
        else:
            failed_files.append(representative_file.name)
            print_error(f"    Failed: {representative_file.name}")

    if failed_files:
        print_warning(f"\nFailed files for {vendor_name}:")
        for f in failed_files:
            print_warning(f"  - {f}")

    return success_count


def generate_code(vendor_filter: str = None):
    """Generate C++ code from all databases"""

    print_header("Generating C++ Code from Databases")

    cmd = [
        sys.executable,
        str(SCRIPT_DIR / "generate_all.py"),
    ]

    if vendor_filter:
        cmd.extend(["--vendor", vendor_filter])
    else:
        cmd.append("--all")

    result = subprocess.run(cmd)

    if result.returncode != 0:
        print_error("Code generation failed")
        return False

    return True


def main():
    parser = argparse.ArgumentParser(
        description="Complete SVD to C++ generation pipeline for all vendors",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Process all vendors
  python3 update_all_vendors.py --all

  # Process specific vendor
  python3 update_all_vendors.py --vendor STMicro

  # Process specific family
  python3 update_all_vendors.py --vendor STMicro --family stm32f1

  # Parse only (no code generation)
  python3 update_all_vendors.py --vendor Nordic --parse-only

  # Generate code only (skip parsing)
  python3 update_all_vendors.py --vendor STMicro --generate-only
        """
    )

    parser.add_argument('--all', action='store_true',
                        help='Process all vendors')
    parser.add_argument('--vendor', '-v',
                        help='Process specific vendor (e.g., STMicro, Nordic)')
    parser.add_argument('--family', '-f',
                        help='Process specific family (e.g., stm32f1, nrf52)')
    parser.add_argument('--parse-only', action='store_true',
                        help='Only parse SVD files, skip code generation')
    parser.add_argument('--generate-only', action='store_true',
                        help='Only generate code from existing databases')
    parser.add_argument('--list', action='store_true',
                        help='List available vendors and families')

    args = parser.parse_args()

    # Validate arguments
    if not args.all and not args.vendor and not args.list:
        parser.print_help()
        sys.exit(1)

    if args.family and not args.vendor:
        print_error("--family requires --vendor")
        sys.exit(1)

    print_header("Alloy MCU Update Pipeline - SVD to C++ Code Generator")

    # List mode
    if args.list:
        vendors = find_svd_files()
        print_info(f"Found {len(vendors)} vendors:\n")

        for vendor_name, svd_files in sorted(vendors.items()):
            families = set()
            for svd in svd_files:
                family = extract_family_from_name(svd.stem, vendor_name)
                families.add(family)

            print(f"  {Colors.BOLD}{vendor_name}{Colors.ENDC} ({len(svd_files)} MCUs)")
            print(f"    Families: {', '.join(sorted(families))}")

        return 0

    # Generate-only mode
    if args.generate_only:
        if generate_code(args.vendor):
            print_success("\nCode generation complete!")
            return 0
        else:
            return 1

    # Parse SVD files
    if not args.generate_only:
        # Create database directory
        DATABASE_DIR.mkdir(parents=True, exist_ok=True)

        # Find SVD files
        vendors = find_svd_files(args.vendor, args.family)

        if not vendors:
            if args.vendor:
                print_error(f"No SVD files found for vendor: {args.vendor}")
            else:
                print_error("No SVD files found")
            return 1

        print_info(f"Found {len(vendors)} vendor(s) to process\n")

        # Process each vendor
        total_success = 0
        for vendor_name, svd_files in vendors.items():
            count = process_vendor(vendor_name, svd_files, args.family)
            total_success += count

        print_header("Parsing Complete")
        print_success(f"Successfully parsed {total_success} database(s)")

    # Generate code unless parse-only
    if not args.parse_only:
        if generate_code(args.vendor):
            print_success("\n✓ Pipeline complete!")
        else:
            print_error("\n✗ Pipeline failed during code generation")
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
