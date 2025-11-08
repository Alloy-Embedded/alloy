#!/usr/bin/env python3
"""
Generate Pin Alternate Function Mappings

This generator creates type-safe alternate function (AF) mappings for MCU pins.
Since CMSIS-SVD files typically don't include pin AF data, this generator can:

1. Use vendor-specific pin data files (JSON/YAML)
2. Generate from existing pin headers
3. Create templates for manual AF definition

Part of Phase 3: Pin Alternate Function Generator
"""

import sys
import json
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple
from dataclasses import dataclass, field as dc_field

# Add parent to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, logger
from cli.core.config import normalize_name, BOARD_MCUS
from cli.core.paths import get_mcu_output_dir, ensure_dir
from cli.parsers.generic_svd import parse_svd, SVDDevice
from cli.core.progress import get_global_tracker


# ============================================================================
# DATA STRUCTURES
# ============================================================================

@dataclass
class PinFunction:
    """Represents an alternate function for a pin"""
    pin_name: str  # e.g., "PA9"
    peripheral: str  # e.g., "USART1"
    signal: str  # e.g., "TX"
    af_number: int  # AF number (0-15 for STM32, varies by vendor)


@dataclass
class PinFunctionDatabase:
    """Database of all pin alternate functions for an MCU"""
    mcu_name: str
    vendor: str
    family: str
    pin_functions: List[PinFunction] = dc_field(default_factory=list)

    def get_functions_for_pin(self, pin_name: str) -> List[PinFunction]:
        """Get all alternate functions for a specific pin"""
        return [pf for pf in self.pin_functions if pf.pin_name == pin_name]

    def get_pins_for_peripheral(self, peripheral: str) -> List[PinFunction]:
        """Get all pins that can be used with a peripheral"""
        return [pf for pf in self.pin_functions if pf.peripheral == peripheral]

    def get_unique_peripherals(self) -> Set[str]:
        """Get set of all peripherals with pin functions"""
        return set(pf.peripheral for pf in self.pin_functions)

    def get_unique_signals(self) -> Set[str]:
        """Get set of all signal names"""
        return set(pf.signal for pf in self.pin_functions)


# ============================================================================
# PIN FUNCTION DATA LOADING
# ============================================================================

def load_pin_functions_from_json(json_path: Path) -> Optional[PinFunctionDatabase]:
    """
    Load pin function database from JSON file.

    JSON format:
    {
        "mcu": "STM32F103C8",
        "vendor": "st",
        "family": "stm32f1",
        "pin_functions": [
            {"pin": "PA9", "peripheral": "USART1", "signal": "TX", "af": 7},
            {"pin": "PA10", "peripheral": "USART1", "signal": "RX", "af": 7},
            ...
        ]
    }
    """
    try:
        with open(json_path, 'r') as f:
            data = json.load(f)

        db = PinFunctionDatabase(
            mcu_name=data['mcu'],
            vendor=data['vendor'],
            family=data['family']
        )

        for pf_data in data.get('pin_functions', []):
            db.pin_functions.append(PinFunction(
                pin_name=pf_data['pin'],
                peripheral=pf_data['peripheral'],
                signal=pf_data['signal'],
                af_number=pf_data['af']
            ))

        return db

    except Exception as e:
        logger.error(f"Failed to load pin functions from {json_path}: {e}")
        return None


def create_example_pin_function_database(device: SVDDevice) -> PinFunctionDatabase:
    """
    Create an example/template pin function database.

    This is used when no pin function data file exists, to generate
    a template that users can fill in manually.
    """
    db = PinFunctionDatabase(
        mcu_name=device.name,
        vendor=device.vendor_normalized,
        family=device.family
    )

    # Add example entries for documentation purposes
    # Users would replace these with actual MCU pin functions

    if device.vendor_normalized == "st":
        # STM32 example
        db.pin_functions.extend([
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA10", "USART1", "RX", 7),
            PinFunction("PA2", "USART2", "TX", 7),
            PinFunction("PA3", "USART2", "RX", 7),
        ])
    elif device.vendor_normalized == "atmel":
        # Atmel example (different AF numbering)
        db.pin_functions.extend([
            PinFunction("PA22", "SERCOM3", "TX", 2),  # SERCOM pad 0
            PinFunction("PA23", "SERCOM3", "RX", 2),  # SERCOM pad 1
        ])

    return db


# ============================================================================
# CODE GENERATION
# ============================================================================

def generate_pin_functions_header(db: PinFunctionDatabase, device: SVDDevice) -> str:
    """
    Generate pin_functions.hpp with type-safe AF mappings.
    """
    vendor_ns = device.vendor_normalized
    family_ns = normalize_name(device.family)
    mcu_ns = normalize_name(device.name)

    # Header
    content = f"""/// Auto-generated pin alternate function mappings
/// Device: {device.name}
/// Vendor: {device.vendor}
///
/// This file provides compile-time type-safe alternate function mapping
/// for MCU pins. Use the AF<Pin, Function> template to get the AF number.
///
/// DO NOT EDIT - Generated by Alloy Code Generator

#pragma once

#include <cstdint>

namespace alloy::hal::{vendor_ns}::{family_ns}::{mcu_ns}::pin_functions {{

// ============================================================================
// PERIPHERAL SIGNAL TAG TYPES
// ============================================================================

"""

    # Generate peripheral signal tag structs
    signals = sorted(db.get_unique_signals())
    peripherals = sorted(db.get_unique_peripherals())

    content += "// Peripheral signal tags for compile-time type safety\n"
    for peripheral in peripherals:
        for signal in signals:
            # Check if this peripheral/signal combination exists
            exists = any(
                pf.peripheral == peripheral and pf.signal == signal
                for pf in db.pin_functions
            )
            if exists:
                content += f"struct {peripheral}_{signal} {{}};\n"

    content += "\n"

    # Generate AlternateFunction template
    content += """// ============================================================================
// ALTERNATE FUNCTION TEMPLATE
// ============================================================================

/// Template for alternate function mapping
/// @tparam Pin Pin number constant
/// @tparam Function Peripheral signal tag type
template<uint8_t Pin, typename Function>
struct AlternateFunction;

"""

    # Generate specializations grouped by pin
    content += "// ============================================================================\n"
    content += "// ALTERNATE FUNCTION SPECIALIZATIONS\n"
    content += "// ============================================================================\n\n"

    # Group by pin for better readability
    pins_with_functions = {}
    for pf in db.pin_functions:
        if pf.pin_name not in pins_with_functions:
            pins_with_functions[pf.pin_name] = []
        pins_with_functions[pf.pin_name].append(pf)

    for pin_name in sorted(pins_with_functions.keys()):
        content += f"// {pin_name} alternate functions\n"

        for pf in sorted(pins_with_functions[pin_name], key=lambda x: x.af_number):
            # Assuming we have pin number from pins.hpp
            # For now, use the pin name itself
            content += f"template<>\n"
            content += f"struct AlternateFunction<{pin_name}, {pf.peripheral}_{pf.signal}> {{\n"
            content += f"    static constexpr uint8_t af_number = {pf.af_number};\n"
            content += f"}};\n\n"

    # Generate convenience alias
    content += """// ============================================================================
// CONVENIENCE ALIAS
// ============================================================================

/// Get AF number for pin/function combination at compile time
///
/// Usage:
///   constexpr uint8_t af = AF<PA9, USART1_TX>;  // Returns 7 (example)
///
/// Compile-time error if combination is invalid.
template<uint8_t Pin, typename Function>
constexpr uint8_t AF = AlternateFunction<Pin, Function>::af_number;

"""

    # Add validation helper
    content += """// ============================================================================
// COMPILE-TIME VALIDATION
// ============================================================================

/// Check if a pin/function combination is valid
///
/// Usage:
///   static_assert(HasAF<PA9, USART1_TX>, "PA9 does not support USART1_TX");
template<uint8_t Pin, typename Function>
concept HasAF = requires {
    { AlternateFunction<Pin, Function>::af_number } -> std::same_as<const uint8_t&>;
};

"""

    content += f"}}  // namespace alloy::hal::{vendor_ns}::{family_ns}::{mcu_ns}::pin_functions\n"

    return content


def generate_pin_function_data_template(device: SVDDevice, output_path: Path) -> bool:
    """
    Generate a JSON template file for pin function data.

    This file can be manually filled in with actual pin function data
    from the MCU datasheet.
    """
    template = {
        "mcu": device.name,
        "vendor": device.vendor_normalized,
        "family": device.family,
        "pin_functions": [
            {
                "pin": "PA9",
                "peripheral": "USART1",
                "signal": "TX",
                "af": 7,
                "_comment": "Replace with actual pin functions from datasheet"
            },
            {
                "pin": "PA10",
                "peripheral": "USART1",
                "signal": "RX",
                "af": 7,
                "_comment": "Add more pin functions as needed"
            }
        ],
        "_instructions": "Fill this file with pin alternate function data from the MCU datasheet"
    }

    try:
        with open(output_path, 'w') as f:
            json.dump(template, f, indent=2)
        logger.info(f"Created pin function template: {output_path}")
        return True
    except Exception as e:
        logger.error(f"Failed to create template: {e}")
        return False


def generate_for_device(svd_path: Path, pin_data_path: Optional[Path] = None,
                       tracker=None) -> bool:
    """
    Generate pin_functions.hpp for a device.

    Args:
        svd_path: Path to SVD file
        pin_data_path: Optional path to pin function JSON data
        tracker: Optional progress tracker

    Returns:
        True if successful
    """
    # Parse SVD
    device = parse_svd(svd_path, auto_classify=True)
    if not device:
        return False

    # Determine output directory
    output_dir = ensure_dir(
        get_mcu_output_dir(
            device.vendor_normalized,
            device.family,
            device.name.lower()
        )
    )

    logger.info(f"Generating pin functions for {device.name} -> {output_dir}")

    # Load or create pin function database
    if pin_data_path and pin_data_path.exists():
        db = load_pin_functions_from_json(pin_data_path)
        if not db:
            logger.warning(f"Failed to load pin data, using example database")
            db = create_example_pin_function_database(device)
    else:
        logger.info(f"No pin data file found, creating example database")
        db = create_example_pin_function_database(device)

        # Generate template for manual filling
        template_path = output_dir / "pin_functions_template.json"
        generate_pin_function_data_template(device, template_path)

    # Generate pin_functions.hpp
    pin_functions_file = output_dir / "pin_functions.hpp"

    try:
        content = generate_pin_functions_header(db, device)
        pin_functions_file.write_text(content)

        logger.info(f"Generated pin_functions.hpp with {len(db.pin_functions)} mappings")
        return True
    except Exception as e:
        logger.error(f"Failed to generate pin_functions.hpp: {e}")
        return False


def generate_for_board_mcus(verbose: bool = False, tracker=None) -> int:
    """
    Generate pin function headers for all board MCUs.

    Args:
        verbose: Enable verbose output
        tracker: Optional progress tracker

    Returns:
        Exit code (0 for success)
    """
    print_header("Generating Pin Alternate Function Mappings for Board MCUs")

    if tracker:
        tracker.set_generator("pin_functions")

    # Use SVD discovery
    from cli.parsers.svd_discovery import discover_all_svds

    print_info(f"Discovering SVD files for {len(BOARD_MCUS)} board MCUs...")
    all_svds = discover_all_svds()

    # Find SVD files for board MCUs
    matching_files = []
    for board_mcu in BOARD_MCUS:
        mcu_normalized = normalize_name(board_mcu)
        possible_names = [
            board_mcu,
            f"{board_mcu}xx",
            mcu_normalized,
            f"{mcu_normalized}xx",
        ]

        found = False
        for possible_name in possible_names:
            if possible_name in all_svds:
                matching_files.append(all_svds[possible_name].file_path)
                if verbose:
                    print_info(f"  ✓ {board_mcu} → {all_svds[possible_name].file_path.name}")
                found = True
                break

        if not found and verbose:
            print_info(f"  ✗ {board_mcu} (no SVD found)")

    print_info(f"Found SVD files for {len(matching_files)}/{len(BOARD_MCUS)} board MCU(s)")

    if not matching_files:
        print_error("No board MCUs found!")
        return 1

    # Generate for each MCU
    success_count = 0
    for svd_file in matching_files:
        if generate_for_device(svd_file, tracker=tracker):
            success_count += 1

    print_success(f"Generated pin functions for {success_count}/{len(matching_files)} MCU(s)")

    print_info("\nNote: Generated example pin function mappings.")
    print_info("For production use, replace with actual data from MCU datasheets.")
    print_info("See generated pin_functions_template.json files.")

    return 0 if success_count == len(matching_files) else 1


def main():
    """Main entry point"""
    import argparse
    from cli.core.config import SVD_DIR

    parser = argparse.ArgumentParser(description='Generate pin alternate function mappings')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--svd', help='Generate for specific SVD file')
    parser.add_argument('--pin-data', help='Path to pin function JSON data file')
    args = parser.parse_args()

    if args.svd:
        # Generate for specific SVD
        svd_path = Path(args.svd)
        if not svd_path.is_absolute():
            svd_path = SVD_DIR / svd_path

        if not svd_path.exists():
            print_error(f"SVD file not found: {svd_path}")
            return 1

        pin_data_path = Path(args.pin_data) if args.pin_data else None
        success = generate_for_device(svd_path, pin_data_path)
        return 0 if success else 1
    else:
        # Generate for all board MCUs
        return generate_for_board_mcus(args.verbose)


if __name__ == '__main__':
    sys.exit(main())
