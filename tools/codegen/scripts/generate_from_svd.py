#!/usr/bin/env python3
"""
Generate startup.cpp and peripherals.hpp from SVD files

This script processes all SVD files and generates:
- startup.cpp: MCU startup code with vector table
- peripherals.hpp: Peripheral register definitions

Output: src/hal/vendors/{vendor}/{family}/{mcu}/
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import xml.etree.ElementTree as ET

SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent.parent
SVD_DIR = SCRIPT_DIR.parent / "svd" / "upstream" / "cmsis-svd-data" / "data"
OUTPUT_DIR = REPO_ROOT / "src" / "hal" / "vendors"

# Vendor name mappings for cleaner directory names
VENDOR_MAPPING = {
    "STMicroelectronics": "st",
    "STMicro": "st",
    "Atmel": "atmel",
    "Microchip Technology": "atmel",
    "Microchip": "atmel",
    "NXP": "nxp",
    "Freescale": "nxp",
    "Freescale Semiconductor, Inc.": "nxp",
    "Nordic Semiconductor": "nordic",
    "Espressif Systems (Shanghai) Co., Ltd.": "espressif",
    "Espressif Community": "espressif",
    "Raspberry Pi": "raspberrypi",
    "RaspberryPi": "raspberrypi",
    "Texas Instruments": "ti",
    "TexasInstruments": "ti",
    "Silicon Labs": "silabs",
    "SiliconLabs": "silabs",
    "Infineon": "infineon",
    "Infineon Technologies AG": "infineon",
    "Renesas": "renesas",
    "Renesas Electronics Corporation": "renesas",
    "Toshiba": "toshiba",
    "Fujitsu": "fujitsu",
    "Spansion": "spansion",
    "Holtek": "holtek",
    "Nuvoton": "nuvoton",
    "GigaDevice": "gigadevice",
    "Cypress": "cypress",
    "Cypress Semiconductor": "cypress",
    "Keil": "arm",
    "ARM Ltd": "arm",
    "ARM Sample": "arm",
    "SiFive": "sifive",
    "SiFive Community": "sifive",
    "Canaan Inc.": "canaan",
    "Allwinner": "allwinner",
    "Alif Semiconductor": "alif",
}

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

def print_error(msg: str):
    print(f"{Colors.FAIL}✗{Colors.ENDC} {msg}")

def print_header(msg: str):
    print(f"\n{Colors.BOLD}{'='*60}{Colors.ENDC}")
    print(f"{Colors.BOLD}{msg}{Colors.ENDC}")
    print(f"{Colors.BOLD}{'='*60}{Colors.ENDC}\n")


def normalize_name(name: str) -> str:
    """Normalize name to lowercase with underscores"""
    name = name.lower()
    name = re.sub(r'[^a-z0-9]+', '_', name)
    name = name.strip('_')
    name = re.sub(r'_+', '_', name)
    return name


def map_vendor_name(vendor: str) -> str:
    """Map vendor name to clean directory name"""
    for key, value in VENDOR_MAPPING.items():
        if key.lower() in vendor.lower():
            return value
    return normalize_name(vendor)


def detect_family(device_name: str) -> str:
    """Detect MCU family from device name"""
    device_lower = device_name.lower()

    # STM32 families
    if 'stm32' in device_lower:
        if match := re.search(r'stm32([a-z]\d)', device_lower):
            return f"stm32{match.group(1)}"
        return "stm32"

    # SAME/SAMV/SAMD families
    if match := re.search(r'(sam[edv]\d+)', device_lower):
        return match.group(1)

    # RP2040
    if 'rp2040' in device_lower:
        return 'rp2040'

    # ESP32 families
    if 'esp32' in device_lower:
        if 'c3' in device_lower:
            return 'esp32_c3'
        if 'c2' in device_lower:
            return 'esp32_c2'
        if 'c6' in device_lower:
            return 'esp32_c6'
        if 's2' in device_lower:
            return 'esp32_s2'
        if 's3' in device_lower:
            return 'esp32_s3'
        if 'h2' in device_lower:
            return 'esp32_h2'
        if 'p4' in device_lower:
            return 'esp32_p4'
        return 'esp32'

    # Nordic nRF families (nRF51, nRF52, nRF53, nRF91, etc.)
    if match := re.search(r'(nrf\d{2})', device_lower):
        return match.group(1)

    # Texas Instruments families
    if match := re.search(r'(msp\d+)', device_lower):
        # MSP430, MSP432, etc
        return match.group(1)
    if match := re.search(r'(tm4c\d+)', device_lower):
        # TM4C123, TM4C129, etc -> tm4c12x, tm4c
        base = re.search(r'(tm4c\d{2})', device_lower)
        return base.group(1) if base else 'tm4c'
    if match := re.search(r'(cc\d{2})', device_lower):
        # CC13x0, CC26x0, etc
        return match.group(1)

    # Silicon Labs families
    if match := re.search(r'(efm\d+)', device_lower):
        # EFM32, EFM8
        return match.group(1)
    if match := re.search(r'(efr\d+)', device_lower):
        # EFR32BG24 -> efr32
        return match.group(1)

    # NXP/Freescale Kinetis families
    if match := re.search(r'(mk\d{2})', device_lower):
        # MK64F12 -> mk64, MKL26Z4 -> mkl26
        return match.group(1)
    if match := re.search(r'(mkl\d{2})', device_lower):
        # Kinetis L series
        return match.group(1)
    if match := re.search(r'(mkv\d{2})', device_lower):
        # Kinetis V series
        return match.group(1)
    if match := re.search(r'(mkw\d{2})', device_lower):
        # Kinetis W series (wireless)
        return match.group(1)

    # NXP LPC families
    if match := re.search(r'(lpc\d{2,4})', device_lower):
        # LPC1102, LPC5410, etc -> lpc11xx, lpc54xx
        base = re.search(r'(lpc\d{2})', device_lower)
        return base.group(1) + 'xx' if base else 'lpc'

    # NXP i.MX RT families
    if match := re.search(r'(mimxrt\d{4})', device_lower):
        # MIMXRT1011, MIMXRT1062 -> mimxrt10xx
        base = re.search(r'(mimxrt\d{2})', device_lower)
        return base.group(1) + 'xx' if base else 'imxrt'

    # Infineon XMC families
    if match := re.search(r'(xmc\d+)', device_lower):
        return match.group(1)

    # Renesas families
    if match := re.search(r'(r7fa\d)', device_lower):
        # RA family
        return match.group(1)

    # Default: use first part of name
    parts = re.split(r'[^a-z0-9]+', device_lower)
    return parts[0] if parts else 'unknown'


def parse_svd(svd_path: Path) -> Optional[Dict]:
    """Parse SVD file and extract basic info"""
    try:
        tree = ET.parse(svd_path)
        root = tree.getroot()

        # root IS the device element in SVD files
        name_elem = root.find('name')
        vendor_elem = root.find('vendor')
        cpu_elem = root.find('cpu')

        if name_elem is None:
            return None

        device_name = name_elem.text.strip()

        # Get vendor from XML or infer from path
        if vendor_elem is not None:
            vendor = vendor_elem.text.strip()
        else:
            # Infer from path: tools/.../data/VendorName/.../device.svd
            # Walk up the path until we find the vendor directory (child of 'data')
            current = svd_path.parent
            while current.name != 'data' and current.parent.name != '':
                if current.parent.name == 'data':
                    vendor = current.name
                    break
                current = current.parent
            else:
                vendor = "Unknown"

        # Get CPU name
        cpu_name = None
        if cpu_elem is not None:
            cpu_name_elem = cpu_elem.find('name')
            if cpu_name_elem is not None:
                cpu_name = cpu_name_elem.text.strip()

        # Get peripherals and interrupts
        interrupts = []
        peripheral_list = []
        peripherals_xml = root.findall('.//peripheral')

        for peripheral in peripherals_xml:
            # Extract peripheral info
            name_elem = peripheral.find('name')
            base_addr_elem = peripheral.find('baseAddress')
            desc_elem = peripheral.find('description')

            if name_elem is not None and base_addr_elem is not None:
                try:
                    peripheral_info = {
                        'name': name_elem.text.strip(),
                        'base': int(base_addr_elem.text, 0),
                        'description': desc_elem.text.strip() if desc_elem is not None else ''
                    }
                    peripheral_list.append(peripheral_info)
                except (ValueError, AttributeError):
                    pass

            # Extract interrupts for this peripheral
            for interrupt in peripheral.findall('interrupt'):
                name_elem = interrupt.find('name')
                value_elem = interrupt.find('value')
                if name_elem is not None and value_elem is not None:
                    try:
                        interrupts.append({
                            'name': name_elem.text.strip(),
                            'value': int(value_elem.text)
                        })
                    except (ValueError, AttributeError):
                        pass

        # Sort by IRQ number and peripheral name
        interrupts.sort(key=lambda x: x['value'])
        peripheral_list.sort(key=lambda x: x['name'])

        # Get memory info
        address_blocks = []
        for peripheral in peripherals_xml:
            for block in peripheral.findall('.//addressBlock'):
                offset_elem = block.find('offset')
                size_elem = block.find('size')
                usage_elem = block.find('usage')
                if offset_elem is not None and size_elem is not None:
                    try:
                        address_blocks.append({
                            'offset': int(offset_elem.text, 0),
                            'size': int(size_elem.text, 0),
                            'usage': usage_elem.text if usage_elem is not None else 'registers'
                        })
                    except (ValueError, AttributeError):
                        pass

        return {
            'name': device_name,
            'vendor': vendor,
            'cpu': cpu_name,
            'interrupts': interrupts,
            'peripherals': peripheral_list,
            'address_blocks': address_blocks
        }

    except Exception as e:
        print_error(f"Failed to parse {svd_path.name}: {e}")
        return None


def generate_startup_cpp(device_info: Dict, output_path: Path):
    """Generate startup.cpp file"""
    device_name = device_info['name']
    interrupts = device_info['interrupts']

    # Generate vector table entries
    handlers_set = set()  # Use set to deduplicate
    handlers = []
    vector_table = []

    for irq in interrupts:
        handler_name = f"{irq['name']}_Handler"
        if handler_name not in handlers_set:
            handlers.append(handler_name)
            handlers_set.add(handler_name)
        vector_table.append(f"    {handler_name},  // IRQ {irq['value']}: {irq['name']}")

    content = f'''/// Auto-generated startup code for {device_name}
/// Generated by Alloy SVD Code Generator
/// DO NOT EDIT - Regenerate from SVD if needed

#include <cstdint>
#include <cstring>

// Linker script symbols
extern uint32_t _sidata;  // Start of .data in flash
extern uint32_t _sdata;   // Start of .data in RAM
extern uint32_t _edata;   // End of .data in RAM
extern uint32_t _sbss;    // Start of .bss
extern uint32_t _ebss;    // End of .bss

// Stack top (defined in linker script)
extern uint32_t _estack;

// User application entry point
extern "C" int main();

// System initialization (weak, can be overridden)
extern "C" void SystemInit() __attribute__((weak));
extern "C" void SystemInit() {{}}

// Default handler for unhandled interrupts
extern "C" [[noreturn]] void Default_Handler() {{
    while (true) {{
        // Trap
    }}
}}

// Interrupt handlers (weak, can be overridden)
'''

    # Add weak handler declarations
    for handler in handlers:
        content += f'extern "C" void {handler}() __attribute__((weak, alias("Default_Handler")));\n'

    content += '''
// Reset Handler - Entry point after reset
extern "C" [[noreturn]] void Reset_Handler() {
    // 1. Copy .data section from Flash to RAM
    uint32_t* src = &_sidata;
    uint32_t* dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    // 2. Zero out .bss section
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    // 3. Call system initialization
    SystemInit();

    // 4. Call static constructors
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();
    for (auto ctor = __init_array_start; ctor < __init_array_end; ++ctor) {
        (*ctor)();
    }

    // 5. Call main
    main();

    // 6. If main returns, loop forever
    while (true) {}
}

// Vector table
__attribute__((section(".isr_vector"), used))
void (* const vector_table[])() = {
    reinterpret_cast<void (*)()>(&_estack),  // Initial stack pointer
    Reset_Handler,                            // Reset handler
'''

    # Add all interrupt handlers
    content += '\n'.join(vector_table)
    content += '\n};\n'

    output_path.write_text(content)


def generate_peripherals_hpp(device_info: Dict, output_path: Path, namespace: str):
    """Generate peripherals.hpp file with complete peripheral definitions"""
    device_name = device_info['name']
    vendor = device_info.get('vendor', 'Unknown')
    peripherals = device_info.get('peripherals', [])
    interrupts = device_info.get('interrupts', [])

    # Build peripheral address constants
    peripheral_defs = []
    for periph in peripherals:
        name = periph['name']
        base = periph['base']
        desc = periph['description']
        # Format: constexpr uintptr_t NAME = 0xADDRESS;  // Description
        peripheral_defs.append(f"constexpr uintptr_t {name} = 0x{base:08X};  // {desc}")

    # Build peripheral ID constants from interrupts
    id_defs = []
    seen_ids = set()
    for irq in interrupts:
        # IRQ name often contains peripheral name (e.g., "UART0_IRQn" -> ID: 7)
        # Extract peripheral name from IRQ name
        irq_name = irq['name']
        irq_value = irq['value']

        # Common patterns: UART0_IRQn, PIOA_IRQHandler, SPI0 -> UART0, PIOA, SPI0
        periph_name = irq_name.replace('_IRQn', '').replace('_IRQHandler', '').replace('_Handler', '')

        # Only add if we have a peripheral with this name
        periph_exists = any(p['name'] == periph_name for p in peripherals)

        if periph_exists and periph_name not in seen_ids:
            id_defs.append(f"constexpr uint8_t {periph_name} = {irq_value};    // {periph_name}")
            seen_ids.add(periph_name)

    content = f'''/// Auto-generated peripheral definitions for {device_name}
/// Generated by Alloy Code Generator from CMSIS-SVD
///
/// Device:  {device_name}
/// Vendor:  {vendor}
/// Family:  {namespace}
///
/// DO NOT EDIT - Regenerate from SVD if needed

#ifndef ALLOY_GENERATED_{device_name.upper()}_PERIPHERALS_HPP
#define ALLOY_GENERATED_{device_name.upper()}_PERIPHERALS_HPP

#include <cstdint>

namespace alloy::generated::{namespace} {{

// ============================================================================
// PERIPHERAL BASE ADDRESSES
// ============================================================================

namespace peripherals {{
{chr(10).join(peripheral_defs) if peripheral_defs else "// No peripherals found in SVD"}
}}  // namespace peripherals

// ============================================================================
// PERIPHERAL IDs (for Clock Enable/Disable)
// ============================================================================

namespace id {{
{chr(10).join(id_defs) if id_defs else "// No interrupt IDs found in SVD"}
}}  // namespace id

// ============================================================================
// MEMORY MAP
// ============================================================================

namespace memory {{
constexpr uintptr_t SDRAMC = 0x40084000;
}}  // namespace memory

}}  // namespace alloy::generated::{namespace}

#endif  // ALLOY_GENERATED_{device_name.upper()}_PERIPHERALS_HPP
'''

    output_path.write_text(content)


def process_svd_file(svd_path: Path, args) -> bool:
    """Process a single SVD file"""
    # Parse SVD
    device_info = parse_svd(svd_path)
    if not device_info:
        return False

    device_name = device_info['name']
    vendor = device_info['vendor']

    # Determine paths
    vendor_dir = map_vendor_name(vendor)
    family = detect_family(device_name)
    mcu_dir = normalize_name(device_name)

    # Important: Don't nest family inside itself
    # Structure should be: vendor/family/mcu (NOT vendor/family/family)
    # If mcu_dir starts with family, it means we already have the family in the name
    # Example: family="nrf52", mcu_dir="nrf52820" -> correct: nordic/nrf52/nrf52820
    # Example: family="same70", mcu_dir="atsame70q21" -> correct: atmel/same70/atsame70q21

    output_path = OUTPUT_DIR / vendor_dir / family / mcu_dir
    output_path.mkdir(parents=True, exist_ok=True)

    if args.verbose:
        print_info(f"Processing {device_name} -> {vendor_dir}/{family}/{mcu_dir}")

    # Generate files
    try:
        generate_startup_cpp(device_info, output_path / "startup.cpp")
        generate_peripherals_hpp(device_info, output_path / "peripherals.hpp",
                                normalize_name(device_name))
        return True
    except Exception as e:
        print_error(f"Failed to generate for {device_name}: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description='Generate code from SVD files')
    parser.add_argument('--vendor', help='Process only specific vendor')
    parser.add_argument('--mcu', help='Process only specific MCU(s), comma-separated (e.g. "STM32F103,ATSAME70")')
    parser.add_argument('--limit', type=int, help='Limit number of files to process (for testing)')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be generated without writing files')
    parser.add_argument('--boards-only', action='store_true', help='Generate only for MCUs with board configs')

    args = parser.parse_args()

    print_header("Alloy MCU Code Generator - SVD to C++")

    # MCUs we have board configurations for
    BOARD_MCUS = [
        "ATSAMD21G18A",   # arduino_zero
        "STM32F103",      # bluepill (family)
        "ESP32",          # esp32_devkit
        "RP2040",         # rp_pico / rp2040_zero
        "ATSAME70Q21",    # same70_xpld
        "ATSAMV71Q21",    # samv71_xult
        "STM32F407",      # stm32f407vg (family)
        "STM32F746",      # stm32f746disco (family)
    ]

    # Find all SVD files
    svd_files = list(SVD_DIR.rglob("*.svd"))

    if args.boards_only:
        # Filter for only board MCUs
        filtered_files = []
        for svd in svd_files:
            try:
                tree = ET.parse(svd)
                root = tree.getroot()
                name_elem = root.find('name')
                if name_elem is not None:
                    device_name = name_elem.text.strip().upper()
                    # Check if device matches any board MCU (exact or family match)
                    for board_mcu in BOARD_MCUS:
                        if board_mcu.upper() in device_name or device_name.startswith(board_mcu.upper()):
                            filtered_files.append(svd)
                            break
            except:
                pass
        svd_files = filtered_files
    elif args.mcu:
        # Filter for specific MCU(s)
        mcu_list = [m.strip().upper() for m in args.mcu.split(',')]
        filtered_files = []
        for svd in svd_files:
            try:
                tree = ET.parse(svd)
                root = tree.getroot()
                name_elem = root.find('name')
                if name_elem is not None:
                    device_name = name_elem.text.strip().upper()
                    for mcu in mcu_list:
                        if mcu in device_name or device_name.startswith(mcu):
                            filtered_files.append(svd)
                            break
            except:
                pass
        svd_files = filtered_files
    elif args.vendor:
        svd_files = [f for f in svd_files if args.vendor.lower() in str(f.parent).lower()]

    if args.limit:
        svd_files = svd_files[:args.limit]

    print_info(f"Found {len(svd_files)} SVD files to process")

    # Process each SVD
    success_count = 0
    fail_count = 0

    for svd_file in svd_files:
        if args.dry_run:
            device_info = parse_svd(svd_file)
            if device_info:
                vendor_dir = map_vendor_name(device_info['vendor'])
                family = detect_family(device_info['name'])
                mcu_dir = normalize_name(device_info['name'])
                print_info(f"Would generate: {vendor_dir}/{family}/{mcu_dir}")
                success_count += 1
            else:
                fail_count += 1
        else:
            if process_svd_file(svd_file, args):
                success_count += 1
            else:
                fail_count += 1

    # Summary
    print_header("Generation Summary")
    print_success(f"Successfully processed: {success_count}")
    if fail_count > 0:
        print_error(f"Failed: {fail_count}")

    return 0 if fail_count == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
