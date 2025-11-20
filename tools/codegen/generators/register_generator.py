#!/usr/bin/env python3
"""
Generate Register Structure Headers from SVD

This generator creates type-safe register structure definitions from parsed SVD data.
It generates:
1. Register structures with volatile members
2. Peripheral-specific register definitions
3. Bit field constants for each register field

Part of Phase 1: Enhanced SVD Parser + Basic Register Generation
"""

import sys
from pathlib import Path
from typing import List, Optional, Set

# Add parent to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from core.logger import print_header, print_success, print_error, print_info, logger
from core.config import normalize_name
from core.paths import get_mcu_output_dir, ensure_dir, get_family_dir, get_generated_output_dir
from cli.parsers.generic_svd import parse_svd, SVDDevice, Peripheral, Register, RegisterField
from core.progress import get_global_tracker
import re


def sanitize_description(description: str) -> str:
    """
    Sanitize a description string to be safe for C++ comments.

    Removes problematic newlines, extra whitespace, and ensures
    descriptions don't break out of comment blocks.

    Args:
        description: Original description from SVD

    Returns:
        Cleaned description suitable for C++ comments
    """
    if not description:
        return ""

    # Replace newlines and multiple spaces with single space
    cleaned = re.sub(r'\s+', ' ', description)

    # Remove leading/trailing whitespace
    cleaned = cleaned.strip()

    # Remove any comment-breaking sequences (*/,  /*)
    cleaned = cleaned.replace('*/', '').replace('/*', '')

    return cleaned


def sanitize_identifier(name: str) -> str:
    """
    Sanitize a name to be a valid C++ identifier.

    C++ identifiers cannot start with a digit, so we prefix with an underscore
    if needed. Also handles other invalid characters.

    Args:
        name: Original identifier name

    Returns:
        Valid C++ identifier

    Examples:
        "1_BANK" -> "_1_BANK"
        "8_BYTE" -> "_8_BYTE"
        "NORMAL" -> "NORMAL"
    """
    if not name:
        return "INVALID"

    # If starts with a digit, prefix with underscore
    if name[0].isdigit():
        name = '_' + name

    # Replace any remaining invalid characters with underscore
    # C++ identifiers can only contain: a-z, A-Z, 0-9, _
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)

    return name


def sanitize_namespace_name(name: str) -> str:
    """
    Sanitize a register or field name to be a valid C++ namespace identifier.

    Handles register arrays by removing array syntax (e.g., "ABCDSR[2]" -> "abcdsr").
    This is necessary because C++ namespaces cannot contain brackets.
    Also sanitizes C++ reserved keywords by appending underscore.

    Args:
        name: Original register/field name from SVD (may contain array syntax)

    Returns:
        Valid C++ namespace identifier (lowercase, no array brackets, no reserved keywords)

    Examples:
        "ABCDSR[2]" -> "abcdsr"
        "GPIO_PINn[16]" -> "gpio_pinn"
        "NORMAL_REG" -> "normal_reg"
        "ASM" -> "asm_"
        "CLASS" -> "class_"
    """
    # C++ reserved keywords that cannot be used as identifiers
    CPP_KEYWORDS = {
        'alignas', 'alignof', 'and', 'and_eq', 'asm', 'auto', 'bitand', 'bitor',
        'bool', 'break', 'case', 'catch', 'char', 'char8_t', 'char16_t', 'char32_t',
        'class', 'compl', 'concept', 'const', 'consteval', 'constexpr', 'constinit',
        'const_cast', 'continue', 'co_await', 'co_return', 'co_yield', 'decltype',
        'default', 'delete', 'do', 'double', 'dynamic_cast', 'else', 'enum',
        'explicit', 'export', 'extern', 'false', 'float', 'for', 'friend', 'goto',
        'if', 'inline', 'int', 'long', 'mutable', 'namespace', 'new', 'noexcept',
        'not', 'not_eq', 'nullptr', 'operator', 'or', 'or_eq', 'private', 'protected',
        'public', 'register', 'reinterpret_cast', 'requires', 'return', 'short',
        'signed', 'sizeof', 'static', 'static_assert', 'static_cast', 'struct',
        'switch', 'template', 'this', 'thread_local', 'throw', 'true', 'try',
        'typedef', 'typeid', 'typename', 'union', 'unsigned', 'using', 'virtual',
        'void', 'volatile', 'wchar_t', 'while', 'xor', 'xor_eq'
    }

    # Remove array syntax: [2], [16], etc.
    sanitized = re.sub(r'\[\d+\]', '', name)

    # Remove any remaining bracket artifacts
    sanitized = sanitized.replace('[', '').replace(']', '')

    # Convert to lowercase for namespace convention
    sanitized = sanitized.lower()

    # Check if it's a reserved keyword and append underscore if so
    if sanitized in CPP_KEYWORDS:
        sanitized = sanitized + '_'

    return sanitized


def generate_register_struct(peripheral: Peripheral, device: SVDDevice, family_level: bool = True) -> str:
    """
    Generate C++ register structure for a peripheral.

    Args:
        peripheral: Peripheral with registers
        device: Device information for namespace
        family_level: If True, generate at family level (no MCU in namespace)

    Returns:
        C++ header content with register struct
    """
    vendor_ns = device.vendor_normalized
    family_ns = normalize_name(device.family)
    mcu_ns = normalize_name(device.name)
    periph_ns = normalize_name(peripheral.name)

    # Choose namespace based on generation level
    if family_level:
        namespace = f"alloy::hal::{vendor_ns}::{family_ns}::{periph_ns}"
        device_comment = f"Family: {device.family}"
    else:
        namespace = f"alloy::hal::{vendor_ns}::{family_ns}::{mcu_ns}::{periph_ns}"
        device_comment = f"Device: {device.name}"

    # Header
    periph_desc = sanitize_description(peripheral.description) if peripheral.description else 'Peripheral Registers'
    content = f"""/// Auto-generated register definitions for {peripheral.name}
/// {device_comment}
/// Vendor: {device.vendor}
///
/// DO NOT EDIT - Generated by Alloy Code Generator from CMSIS-SVD

#pragma once

#include <cstdint>

namespace {namespace} {{

// ============================================================================
// {peripheral.name} - {periph_desc}
// Base Address: 0x{peripheral.base_address:08X}
// ============================================================================

"""

    # Generate register structure
    if peripheral.registers:
        content += f"/// {peripheral.name} Register Structure\n"
        content += f"struct {peripheral.name}_Registers {{\n"

        current_offset = 0
        for register in sorted(peripheral.registers, key=lambda r: r.offset):
            # Add padding if needed
            if register.offset > current_offset:
                padding_bytes = register.offset - current_offset
                padding_name = f"RESERVED_{current_offset:04X}"
                content += f"    uint8_t {padding_name}[{padding_bytes}]; ///< Reserved\n"

            # Register comment
            desc = sanitize_description(register.description) if register.description else register.name
            content += f"\n    /// {desc}\n"
            content += f"    /// Offset: 0x{register.offset:04X}\n"
            if register.reset_value is not None:
                content += f"    /// Reset value: 0x{register.reset_value:08X}\n"
            if register.access:
                content += f"    /// Access: {register.access}\n"

            # Register member
            reg_type = f"uint{register.size}_t"
            if register.dim and register.dim > 1:
                # Register array (e.g., ABCDSR[2])
                content += f"    volatile {reg_type} {register.name}[{register.dim}];\n"
                current_offset = register.offset + (register.size // 8) * register.dim
            else:
                # Single register
                content += f"    volatile {reg_type} {register.name};\n"
                current_offset = register.offset + (register.size // 8)

        content += "};\n\n"

        # Static assertion for struct size
        content += f"static_assert(sizeof({peripheral.name}_Registers) >= {current_offset}, "
        content += f"\"{peripheral.name}_Registers size mismatch\");\n\n"

        # Global pointer instance - use inline function instead of constexpr reinterpret_cast
        # (reinterpret_cast is not allowed in constexpr context in C++17)
        content += f"/// {peripheral.name} peripheral instance\n"
        content += f"inline {peripheral.name}_Registers* {peripheral.name}() {{\n"
        content += f"    return reinterpret_cast<{peripheral.name}_Registers*>(0x{peripheral.base_address:08X});\n"
        content += f"}}\n\n"
    else:
        content += f"// No registers defined for {peripheral.name}\n\n"

    # Use the same namespace variable for closing
    content += f"}}  // namespace {namespace}\n"

    return content


def generate_bitfield_definitions(peripheral: Peripheral, device: SVDDevice, family_level: bool = True) -> str:
    """
    Generate bit field definitions for a peripheral's registers.

    Args:
        peripheral: Peripheral with registers and fields
        device: Device information
        family_level: If True, generate at family level (no MCU in namespace)

    Returns:
        C++ header content with bit field definitions
    """
    vendor_ns = device.vendor_normalized
    family_ns = normalize_name(device.family)
    mcu_ns = normalize_name(device.name)
    periph_ns = normalize_name(peripheral.name)

    # Choose namespace based on generation level
    if family_level:
        namespace = f"alloy::hal::{vendor_ns}::{family_ns}::{periph_ns}"
        device_comment = f"Family: {device.family}"
    else:
        namespace = f"alloy::hal::{vendor_ns}::{family_ns}::{mcu_ns}::{periph_ns}"
        device_comment = f"Device: {device.name}"

    # Header
    content = f"""/// Auto-generated bit field definitions for {peripheral.name}
/// {device_comment}
/// Vendor: {device.vendor}
///
/// DO NOT EDIT - Generated by Alloy Code Generator from CMSIS-SVD

#pragma once

#include <cstdint>
#include "hal/utils/bitfield.hpp"

namespace {namespace} {{

using namespace alloy::hal::bitfields;

// ============================================================================
// {peripheral.name} Bit Field Definitions
// ============================================================================

"""

    # Generate bit fields for each register
    for register in sorted(peripheral.registers, key=lambda r: r.offset):
        if not register.fields:
            continue

        # Sanitize register name for namespace (removes array syntax like [2])
        reg_name_lower = sanitize_namespace_name(register.name)
        reg_desc = sanitize_description(register.description) if register.description else register.name
        content += f"/// {register.name} - {reg_desc}\n"
        content += f"namespace {reg_name_lower} {{\n"

        for field in sorted(register.fields, key=lambda f: f.bit_offset):
            # Field comment
            if field.description:
                desc = sanitize_description(field.description)
                content += f"    /// {desc}\n"
            content += f"    /// Position: {field.bit_offset}, Width: {field.bit_width}\n"
            if field.access:
                content += f"    /// Access: {field.access}\n"

            # BitField type alias
            content += f"    using {field.name} = BitField<{field.bit_offset}, {field.bit_width}>;\n"

            # CMSIS-compatible constants
            content += f"    constexpr uint32_t {field.name}_Pos = {field.bit_offset};\n"
            content += f"    constexpr uint32_t {field.name}_Msk = {field.name}::mask;\n"

            # Enumerated values if present
            if field.enum_values:
                field_name_sanitized = sanitize_namespace_name(field.name)
                content += f"    /// Enumerated values for {field.name}\n"
                content += f"    namespace {field_name_sanitized} {{\n"
                for enum_name, enum_value in field.enum_values.items():
                    enum_name_sanitized = sanitize_identifier(enum_name)
                    content += f"        constexpr uint32_t {enum_name_sanitized} = {enum_value};\n"
                content += "    }\n"

            content += "\n"

        content += f"}}  // namespace {reg_name_lower}\n\n"

    # Use the same namespace variable for closing
    content += f"}}  // namespace {namespace}\n"

    return content


def generate_for_peripheral(peripheral: Peripheral, device: SVDDevice,
                            output_dir: Path, family_level: bool = True, tracker=None) -> bool:
    """
    Generate register and bitfield headers for a single peripheral.

    Args:
        peripheral: Peripheral to generate for
        device: Device information
        output_dir: Output directory for generated files
        family_level: If True, generate at family level
        tracker: Optional progress tracker

    Returns:
        True if successful
    """
    # Skip peripherals with no registers
    if not peripheral.registers:
        logger.debug(f"Skipping {peripheral.name} (no registers)")
        return True

    periph_name_lower = peripheral.name.lower()
    success = True

    # Generate register structure
    registers_dir = ensure_dir(output_dir / "registers")
    register_file = registers_dir / f"{periph_name_lower}_registers.hpp"

    try:
        register_content = generate_register_struct(peripheral, device, family_level=family_level)
        register_file.write_text(register_content)
        logger.debug(f"Generated {register_file.name}")
    except Exception as e:
        logger.error(f"Failed to generate register struct for {peripheral.name}: {e}")
        success = False

    # Generate bit field definitions
    bitfields_dir = ensure_dir(output_dir / "bitfields")
    bitfield_file = bitfields_dir / f"{periph_name_lower}_bitfields.hpp"

    try:
        bitfield_content = generate_bitfield_definitions(peripheral, device, family_level=family_level)
        bitfield_file.write_text(bitfield_content)
        logger.debug(f"Generated {bitfield_file.name}")
    except Exception as e:
        logger.error(f"Failed to generate bitfields for {peripheral.name}: {e}")
        success = False

    return success


def generate_for_device(svd_path: Path, family_level: bool = True, tracker=None) -> bool:
    """
    Generate register and bitfield headers for a device.

    Args:
        svd_path: Path to SVD file
        family_level: If True, generate at family level instead of MCU level
        tracker: Optional progress tracker

    Returns:
        True if successful
    """
    # Parse SVD
    device = parse_svd(svd_path, auto_classify=True)
    if not device:
        return False

    # Determine output directory based on generation level
    if family_level:
        # Generate at family level: vendors/{vendor}/{family}/generated/
        output_dir = ensure_dir(
            get_generated_output_dir(device.vendor_normalized, device.family)
        )
        logger.info(f"Generating family-level registers for {device.family} -> {output_dir}")
    else:
        # Generate at MCU level: vendors/{vendor}/{family}/{mcu}/generated/
        output_dir = ensure_dir(
            get_generated_output_dir(
                device.vendor_normalized,
                device.family,
                device.name.lower()
            )
        )
        logger.info(f"Generating MCU-level registers for {device.name} -> {output_dir}")

    # Note: bitfield_utils.hpp is now shared at src/hal/utils/bitfield.hpp
    # No need to copy per-MCU anymore

    # Generate for each peripheral
    success_count = 0
    total_count = 0

    for peripheral in device.peripherals.values():
        if peripheral.registers:  # Only count peripherals with registers
            total_count += 1
            if generate_for_peripheral(peripheral, device, output_dir, family_level=family_level, tracker=tracker):
                success_count += 1

    logger.info(f"Generated registers for {success_count}/{total_count} peripherals")
    return success_count == total_count


def discover_families_with_pins() -> dict:
    """
    Discover families that have MCUs with pins, and select a representative MCU for each.

    Returns:
        Dictionary mapping family key to representative MCU info
        Example: {'atmel/same70': {'vendor': 'atmel', 'family': 'same70', 'mcu': 'atsame70q21b'}}
    """
    from core.config import HAL_VENDORS_DIR

    families_map = {}

    # Find all pin_functions.hpp files
    pin_files = HAL_VENDORS_DIR.rglob("**/pin_functions.hpp")

    for pin_file in pin_files:
        # Path structure: src/hal/vendors/{vendor}/{family}/{mcu}/pin_functions.hpp
        parts = pin_file.parts
        try:
            vendor_idx = parts.index('vendors')
            vendor = parts[vendor_idx + 1]
            family = parts[vendor_idx + 2]
            mcu = parts[vendor_idx + 3]

            family_key = f"{vendor}/{family}"

            # If we haven't seen this family yet, add it
            if family_key not in families_map:
                families_map[family_key] = {
                    'vendor': vendor,
                    'family': family,
                    'mcu': mcu,
                    'path': pin_file.parent
                }
        except (ValueError, IndexError):
            logger.warning(f"Could not parse path: {pin_file}")
            continue

    return families_map


def generate_for_board_mcus(verbose: bool = False, tracker=None) -> int:
    """
    Generate register headers at FAMILY level for all families with MCUs that have pins.

    This new approach generates registers and bitfields once per family, not per MCU,
    since register definitions are family-wide (not MCU-specific).

    Args:
        verbose: Enable verbose output
        tracker: Optional progress tracker

    Returns:
        Exit code (0 for success)
    """
    print_header("Generating Family-Level Register Structures")

    if tracker:
        tracker.set_generator("registers")

    # Discover families that have MCUs with pins
    families_map = discover_families_with_pins()

    if not families_map:
        print_error("No families with pin_functions.hpp found!")
        print_info("Run pin generation first: python3 codegen.py generate --pins")
        return 1

    print_info(f"Found {len(families_map)} family/families with MCUs that have pins:")
    if verbose:
        for family_key, info in families_map.items():
            print_info(f"  • {family_key} (using {info['mcu']} as representative)")

    # Use SVD discovery
    from cli.parsers.svd_discovery import discover_all_svds

    print_info(f"Discovering SVD files...")
    all_svds = discover_all_svds()

    # Find SVD files for representative MCUs
    matching_svds = {}
    for family_key, family_info in families_map.items():
        mcu_name = family_info['mcu']

        # Try common SVD naming patterns
        mcu_upper = mcu_name.upper()
        mcu_normalized = normalize_name(mcu_name)

        # For STM32, try base name without package suffix
        # e.g., stm32f405rg -> STM32F405, stm32f103re -> STM32F103
        import re
        base_stm32_match = re.match(r'(STM32[A-Z]\d+)[A-Z]*\d*', mcu_upper)
        base_stm32 = base_stm32_match.group(1) if base_stm32_match else None

        possible_names = [
            mcu_name,
            mcu_upper,
            f"{mcu_upper}xx",
            mcu_normalized,
            f"{mcu_normalized}xx",
        ]

        # Add STM32 base patterns
        if base_stm32:
            possible_names.extend([
                base_stm32,
                f"{base_stm32}xx",
                base_stm32.lower(),
            ])

        found = False
        for possible_name in possible_names:
            if possible_name in all_svds:
                matching_svds[family_key] = all_svds[possible_name].file_path
                if verbose:
                    print_info(f"  ✓ {family_key} → {all_svds[possible_name].file_path.name}")
                found = True
                break

        if not found:
            if verbose:
                print_info(f"  ✗ {family_key} (no SVD found for {mcu_name})")
            logger.warning(f"No SVD found for family {family_key} (tried {mcu_name})")

    print_info(f"Found SVD files for {len(matching_svds)}/{len(families_map)} family/families")

    if not matching_svds:
        print_error("No families with SVD files found!")
        return 1

    # Generate for each family (using family_level=True)
    success_count = 0
    for family_key, svd_file in matching_svds.items():
        if generate_for_device(svd_file, family_level=True, tracker=tracker):
            success_count += 1

    print_success(f"Generated family-level registers for {success_count}/{len(matching_svds)} family/families")
    return 0 if success_count == len(matching_svds) else 1


def main():
    """Main entry point"""
    import argparse
    from core.config import SVD_DIR

    parser = argparse.ArgumentParser(description='Generate register structures from SVD')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--svd', help='Generate for specific SVD file')
    args = parser.parse_args()

    if args.svd:
        # Generate for specific SVD
        svd_path = Path(args.svd)
        if not svd_path.is_absolute():
            svd_path = SVD_DIR / svd_path

        if not svd_path.exists():
            print_error(f"SVD file not found: {svd_path}")
            return 1

        success = generate_for_device(svd_path)
        return 0 if success else 1
    else:
        # Generate for all board MCUs
        return generate_for_board_mcus(args.verbose)


if __name__ == '__main__':
    sys.exit(main())
