#!/usr/bin/env python3
"""
Database Validator

Validates MCU database JSON files against schema and performs sanity checks.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Any

# Colors for output
class Colors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

def print_success(msg: str):
    print(f"{Colors.OKGREEN}✓{Colors.ENDC} {msg}")

def print_warning(msg: str):
    print(f"{Colors.WARNING}⚠{Colors.ENDC} {msg}")

def print_error(msg: str):
    print(f"{Colors.FAIL}✗{Colors.ENDC} {msg}")


def validate_address(addr: str, name: str) -> List[str]:
    """Validate a hex address string"""
    errors = []

    if not addr.startswith('0x'):
        errors.append(f"{name}: Address must start with '0x': {addr}")

    if len(addr) != 10:  # 0x + 8 hex digits
        errors.append(f"{name}: Address must be 10 characters (0x + 8 hex digits): {addr}")

    try:
        int(addr, 16)
    except ValueError:
        errors.append(f"{name}: Invalid hex address: {addr}")

    return errors


def validate_offset(offset: str, name: str) -> List[str]:
    """Validate a register offset string"""
    errors = []

    if not offset.startswith('0x'):
        errors.append(f"{name}: Offset must start with '0x': {offset}")

    try:
        int(offset, 16)
    except ValueError:
        errors.append(f"{name}: Invalid hex offset: {offset}")

    return errors


def validate_memory_region(region: Dict[str, Any], name: str) -> List[str]:
    """Validate a memory region (flash/RAM)"""
    errors = []

    if 'size_kb' not in region:
        errors.append(f"{name}: Missing 'size_kb' field")
    elif region['size_kb'] <= 0:
        errors.append(f"{name}: size_kb must be positive: {region['size_kb']}")

    if 'base_address' not in region:
        errors.append(f"{name}: Missing 'base_address' field")
    else:
        errors.extend(validate_address(region['base_address'], f"{name}.base_address"))

    return errors


def validate_peripheral(periph: Dict[str, Any], periph_name: str) -> List[str]:
    """Validate a peripheral definition"""
    errors = []

    if 'instances' not in periph:
        errors.append(f"{periph_name}: Missing 'instances' field")
    else:
        for idx, instance in enumerate(periph['instances']):
            if 'name' not in instance:
                errors.append(f"{periph_name}.instances[{idx}]: Missing 'name' field")
            if 'base' not in instance:
                errors.append(f"{periph_name}.instances[{idx}]: Missing 'base' field")
            else:
                errors.extend(validate_address(instance['base'],
                    f"{periph_name}.instances[{idx}].base"))

    if 'registers' not in periph:
        errors.append(f"{periph_name}: Missing 'registers' field")
    else:
        for reg_name, reg in periph['registers'].items():
            if 'offset' not in reg:
                errors.append(f"{periph_name}.registers.{reg_name}: Missing 'offset'")
            else:
                errors.extend(validate_offset(reg['offset'],
                    f"{periph_name}.registers.{reg_name}.offset"))

            if 'size' not in reg:
                errors.append(f"{periph_name}.registers.{reg_name}: Missing 'size'")
            elif reg['size'] not in [8, 16, 32]:
                errors.append(f"{periph_name}.registers.{reg_name}: Invalid size (must be 8, 16, or 32): {reg['size']}")

    return errors


def validate_interrupts(interrupts: Dict[str, Any], mcu_name: str) -> List[str]:
    """Validate interrupt table"""
    errors = []

    if 'count' not in interrupts:
        errors.append(f"{mcu_name}.interrupts: Missing 'count' field")

    if 'vectors' not in interrupts:
        errors.append(f"{mcu_name}.interrupts: Missing 'vectors' field")
    else:
        seen_numbers = set()
        for idx, vector in enumerate(interrupts['vectors']):
            if 'number' not in vector:
                errors.append(f"{mcu_name}.interrupts.vectors[{idx}]: Missing 'number'")
            else:
                num = vector['number']
                if num in seen_numbers:
                    errors.append(f"{mcu_name}.interrupts.vectors[{idx}]: Duplicate vector number {num}")
                seen_numbers.add(num)

            if 'name' not in vector:
                errors.append(f"{mcu_name}.interrupts.vectors[{idx}]: Missing 'name'")

    return errors


def validate_mcu(mcu: Dict[str, Any], mcu_name: str) -> List[str]:
    """Validate a single MCU configuration"""
    errors = []

    # Validate memory regions
    if 'flash' not in mcu:
        errors.append(f"{mcu_name}: Missing 'flash' field")
    else:
        errors.extend(validate_memory_region(mcu['flash'], f"{mcu_name}.flash"))

    if 'ram' not in mcu:
        errors.append(f"{mcu_name}: Missing 'ram' field")
    else:
        errors.extend(validate_memory_region(mcu['ram'], f"{mcu_name}.ram"))

    # Validate peripherals
    if 'peripherals' not in mcu:
        errors.append(f"{mcu_name}: Missing 'peripherals' field")
    else:
        for periph_name, periph in mcu['peripherals'].items():
            errors.extend(validate_peripheral(periph, f"{mcu_name}.peripherals.{periph_name}"))

    # Validate interrupts
    if 'interrupts' not in mcu:
        errors.append(f"{mcu_name}: Missing 'interrupts' field")
    else:
        errors.extend(validate_interrupts(mcu['interrupts'], mcu_name))

    return errors


def validate_database(db_path: Path) -> bool:
    """Validate a database file"""
    print(f"\nValidating: {db_path.name}")
    print("=" * 60)

    # Load JSON
    try:
        with open(db_path) as f:
            db = json.load(f)
    except json.JSONDecodeError as e:
        print_error(f"Invalid JSON: {e}")
        return False
    except Exception as e:
        print_error(f"Failed to read file: {e}")
        return False

    errors = []
    warnings = []

    # Validate top-level fields
    required_fields = ['family', 'architecture', 'vendor', 'mcus']
    for field in required_fields:
        if field not in db:
            errors.append(f"Missing required top-level field: '{field}'")

    # Validate architecture
    valid_archs = [
        'arm-cortex-m0', 'arm-cortex-m0plus', 'arm-cortex-m3',
        'arm-cortex-m4', 'arm-cortex-m4f', 'arm-cortex-m7',
        'arm-cortex-m33', 'rl78', 'xtensa'
    ]
    if 'architecture' in db and db['architecture'] not in valid_archs:
        warnings.append(f"Unknown architecture: {db['architecture']}")

    # Validate each MCU
    if 'mcus' in db:
        if not db['mcus']:
            errors.append("'mcus' field is empty")
        else:
            for mcu_name, mcu in db['mcus'].items():
                errors.extend(validate_mcu(mcu, mcu_name))

    # Print results
    if errors:
        print_error(f"Found {len(errors)} error(s):")
        for error in errors[:10]:  # Show first 10
            print(f"  • {error}")
        if len(errors) > 10:
            print(f"  ... and {len(errors) - 10} more")

    if warnings:
        print_warning(f"Found {len(warnings)} warning(s):")
        for warning in warnings[:5]:
            print(f"  • {warning}")
        if len(warnings) > 5:
            print(f"  ... and {len(warnings) - 5} more")

    if not errors and not warnings:
        print_success("Database is valid!")
        return True
    elif not errors:
        print_success("Database is valid (with warnings)")
        return True
    else:
        print_error("Database validation FAILED")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Validate Alloy MCU database files"
    )
    parser.add_argument('files', nargs='+', type=Path,
                        help='Database JSON file(s) to validate')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')

    args = parser.parse_args()

    all_valid = True
    for db_file in args.files:
        if not db_file.exists():
            print_error(f"File not found: {db_file}")
            all_valid = False
            continue

        if not validate_database(db_file):
            all_valid = False

    print()
    if all_valid:
        print_success(f"All {len(args.files)} database(s) validated successfully!")
        return 0
    else:
        print_error("Some databases failed validation")
        return 1


if __name__ == '__main__':
    sys.exit(main())
