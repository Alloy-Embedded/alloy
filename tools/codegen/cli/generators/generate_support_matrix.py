#!/usr/bin/env python3
"""
Generate MCU Support Matrix

Scans generated MCU code to create a comprehensive support matrix showing
all supported devices and their capabilities.
"""

from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict
from datetime import datetime
import re

REPO_ROOT = Path(__file__).parent.parent.parent
HAL_DIR = REPO_ROOT / "src" / "hal"


@dataclass
class MCUInfo:
    """Information about a supported MCU"""
    name: str
    display_name: str
    vendor: str
    family: str
    package: str
    flash_kb: int
    sram_kb: int
    gpio_pins: int
    uart_count: int
    i2c_count: int
    spi_count: int
    adc_channels: int
    timer_count: int
    dma_channels: int
    has_usb: bool
    has_can: bool
    has_dac: bool
    status: str = "wip"  # "full", "partial", "wip"


def scan_generated_mcus() -> List[MCUInfo]:
    """Scan hal directory for generated MCU code"""
    mcus = []

    if not HAL_DIR.exists():
        return mcus

    # Scan all vendor directories
    for vendor_dir in HAL_DIR.iterdir():
        if not vendor_dir.is_dir() or vendor_dir.name.startswith('.'):
            continue

        vendor_name = vendor_dir.name

        # Look for family directories
        for family_dir in vendor_dir.iterdir():
            if not family_dir.is_dir() or family_dir.name.startswith('.'):
                continue

            family_name = family_dir.name

            # Look for generated/ directory
            generated_dir = family_dir / "generated"
            if not generated_dir.exists():
                continue

            # Each subdirectory is an MCU variant
            for mcu_dir in generated_dir.iterdir():
                if not mcu_dir.is_dir():
                    continue

                # Look for pins.hpp and traits.hpp
                pins_file = mcu_dir / "pins.hpp"
                traits_file = mcu_dir / "traits.hpp"

                if pins_file.exists() and traits_file.exists():
                    try:
                        mcu_info = extract_mcu_info(
                            mcu_dir.name,
                            pins_file,
                            traits_file,
                            vendor_name,
                            family_name
                        )
                        mcus.append(mcu_info)
                    except Exception as e:
                        print(f"Warning: Failed to extract info for {mcu_dir.name}: {e}")

    return mcus


def extract_mcu_info(mcu_name: str, pins_file: Path, traits_file: Path,
                     vendor: str, family: str) -> MCUInfo:
    """Extract MCU information from generated headers"""

    pins_content = pins_file.read_text()
    traits_content = traits_file.read_text()

    # Extract device display name
    match = re.search(r'DEVICE_NAME = "([^"]+)"', traits_content)
    display_name = match.group(1) if match else mcu_name.upper()

    # Extract package
    match = re.search(r'PACKAGE = "([^"]+)"', traits_content)
    package = match.group(1) if match else "Unknown"

    # Extract memory
    match = re.search(r'FLASH_SIZE = (\d+) \* 1024', traits_content)
    flash_kb = int(match.group(1)) if match else 0

    match = re.search(r'SRAM_SIZE = (\d+) \* 1024', traits_content)
    sram_kb = int(match.group(1)) if match else 0

    # Extract pin count
    match = re.search(r'PIN_COUNT = (\d+)', traits_content)
    gpio_pins = int(match.group(1)) if match else 0

    # Extract peripheral counts
    match = re.search(r'UART_COUNT = (\d+)', traits_content)
    uart_count = int(match.group(1)) if match else 0

    match = re.search(r'I2C_COUNT = (\d+)', traits_content)
    i2c_count = int(match.group(1)) if match else 0

    match = re.search(r'SPI_COUNT = (\d+)', traits_content)
    spi_count = int(match.group(1)) if match else 0

    match = re.search(r'ADC_CHANNELS = (\d+)', traits_content)
    adc_channels = int(match.group(1)) if match else 0

    match = re.search(r'TIMER_COUNT = (\d+)', traits_content)
    timer_count = int(match.group(1)) if match else 0

    match = re.search(r'DMA_CHANNELS = (\d+)', traits_content)
    dma_channels = int(match.group(1)) if match else 0

    # Extract feature flags
    has_usb = "HAS_USB = true" in traits_content
    has_can = "HAS_CAN = true" in traits_content
    has_dac = "HAS_DAC = true" in traits_content

    # Determine status (simplified for now)
    status = "wip"  # Default to WIP

    return MCUInfo(
        name=mcu_name,
        display_name=display_name,
        vendor=vendor.upper(),
        family=family.upper(),
        package=package,
        flash_kb=flash_kb,
        sram_kb=sram_kb,
        gpio_pins=gpio_pins,
        uart_count=uart_count,
        i2c_count=i2c_count,
        spi_count=spi_count,
        adc_channels=adc_channels,
        timer_count=timer_count,
        dma_channels=dma_channels,
        has_usb=has_usb,
        has_can=has_can,
        has_dac=has_dac,
        status=status
    )


def generate_markdown_table(mcus: List[MCUInfo]) -> str:
    """Generate markdown table from MCU list"""

    # Group by vendor and family
    grouped = {}
    for mcu in mcus:
        key = (mcu.vendor, mcu.family)
        if key not in grouped:
            grouped[key] = []
        grouped[key].append(mcu)

    # Sort within groups
    for key in grouped:
        grouped[key].sort(key=lambda m: m.name)

    # Generate markdown
    output = ["## 🎯 Supported MCUs\n"]
    output.append("This table is **auto-generated** from the codegen pipeline. "
                  "Run `make update-docs` or `python tools/codegen/generate_support_matrix.py` to refresh.\n")
    output.append(f"*Last updated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}*\n")
    output.append("---\n")

    for (vendor, family), mcu_list in sorted(grouped.items()):
        output.append(f"\n### {vendor}\n")
        output.append(f"\n#### {family} Series\n")

        # Table header
        output.append("| MCU | Package | Flash | RAM | GPIO | UART | I2C | SPI | "
                     "ADC | Timers | DMA | USB | CAN | Status |")
        output.append("|-----|---------|-------|-----|------|------|-----|-----|-----|"
                     "--------|-----|-----|-----|--------|")

        # Table rows
        for mcu in mcu_list:
            usb_icon = "✅" if mcu.has_usb else "❌"
            can_icon = "✅" if mcu.has_can else "❌"
            status_icon = {
                "full": "✅ Full",
                "partial": "⚠️ Partial",
                "wip": "🚧 WIP"
            }.get(mcu.status, "❓ Unknown")

            row = (f"| {mcu.display_name} | "
                   f"{mcu.package} | "
                   f"{mcu.flash_kb}KB | "
                   f"{mcu.sram_kb}KB | "
                   f"{mcu.gpio_pins} | "
                   f"{mcu.uart_count} | "
                   f"{mcu.i2c_count} | "
                   f"{mcu.spi_count} | "
                   f"{mcu.adc_channels}ch | "
                   f"{mcu.timer_count} | "
                   f"{mcu.dma_channels} | "
                   f"{usb_icon} | "
                   f"{can_icon} | "
                   f"{status_icon} |")
            output.append(row)

    # Status legend
    output.append("\n**Status Legend**:")
    output.append("- ✅ **Full**: Complete GPIO, UART, I2C, SPI support with tested examples")
    output.append("- ⚠️ **Partial**: Basic peripherals working, some features untested")
    output.append("- 🚧 **WIP**: Code generated but not fully tested")

    # Statistics
    total = len(mcus)
    full = sum(1 for m in mcus if m.status == "full")
    partial = sum(1 for m in mcus if m.status == "partial")
    wip = sum(1 for m in mcus if m.status == "wip")

    output.append(f"\n**Total Supported MCUs**: {total}")
    if full > 0 or partial > 0:
        output.append(f"({full} fully tested, {partial} partial, {wip} WIP)")

    output.append("\n**Want to add support for a new MCU?** "
                 "See the Custom SVD documentation at `tools/codegen/custom-svd/README.md`")

    return "\n".join(output)


def main():
    print("🔍 Scanning for supported MCUs...")
    mcus = scan_generated_mcus()
    print(f"   Found {len(mcus)} MCUs\n")

    if len(mcus) == 0:
        print("⚠️  No generated MCU code found.")
        print("   Run: python tools/codegen/generate_stm32_pins.py")
        return

    print("📝 Generating support matrix...")
    markdown = generate_markdown_table(mcus)

    # Print to console
    print("\n" + "="*70)
    print(markdown)
    print("="*70 + "\n")

    print("✅ Done!")
    print("\nTo update README.md, add <!-- SUPPORT_MATRIX_START --> and")
    print("<!-- SUPPORT_MATRIX_END --> markers, then run this script with --update-readme")


if __name__ == "__main__":
    main()
