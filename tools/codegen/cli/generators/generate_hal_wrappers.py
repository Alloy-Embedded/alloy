#!/usr/bin/env python3
"""
Generate Universal HAL Wrapper Headers

Generates wrapper headers in src/hal/ that automatically include the correct
peripheral implementation based on CMake-defined MCU macros.

This allows application code to use:
    #include "hal/gpio.hpp"
    #include "hal/uart.hpp"

Instead of:
    #include "hal/platform/stm32f4/gpio.hpp"
    #include "hal/platform/same70/uart.hpp"

The correct implementation is selected at compile-time based on ALLOY_MCU.
"""

import sys
from pathlib import Path
from typing import List, Dict, Set

# Add codegen to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_info, print_error


def discover_peripherals() -> Set[str]:
    """
    Discover all available peripherals by scanning platform directories.

    Returns:
        Set of peripheral names (gpio, uart, spi, etc.)
    """
    repo_root = CODEGEN_DIR.parent.parent
    platform_dir = repo_root / "src" / "hal" / "platform"

    peripherals = set()

    for platform_path in platform_dir.iterdir():
        if not platform_path.is_dir():
            continue

        for periph_file in platform_path.glob("*.hpp"):
            periph_name = periph_file.stem
            peripherals.add(periph_name)

    return peripherals


def discover_platforms_and_mcus() -> Dict[str, List[str]]:
    """
    Discover all platforms and their MCUs by scanning platform directory.

    Returns:
        Dictionary mapping platform name to list of MCU names
    """
    repo_root = CODEGEN_DIR.parent.parent
    platform_dir = repo_root / "src" / "hal" / "platform"

    platforms = {}

    for platform_path in platform_dir.iterdir():
        if not platform_path.is_dir():
            continue

        platform_name = platform_path.name

        # For now, we assume platform-level headers (not per-MCU)
        # This can be extended later if needed
        platforms[platform_name] = []

    return platforms


def generate_wrapper_header(peripheral: str, platforms: Dict[str, List[str]]) -> str:
    """
    Generate universal wrapper header for a peripheral.

    Args:
        peripheral: Peripheral name (e.g., "gpio", "uart")
        platforms: Dictionary of platforms

    Returns:
        Generated header content
    """
    peripheral_upper = peripheral.upper()

    header = f"""/**
 * Universal {peripheral_upper} Header
 *
 * Automatically includes the correct {peripheral_upper} implementation based on the platform
 * defined in the build system (CMake).
 *
 * Usage in application code:
 *   #include "hal/{peripheral}.hpp"
 *
 * The build system defines ALLOY_PLATFORM which determines which implementation to include.
 *
 * Supported platforms: {', '.join(sorted(platforms.keys()))}
 *
 * Example CMake:
 *   set(ALLOY_PLATFORM "stm32f4")
 */

#pragma once

"""

    # Add conditional includes for each platform
    first = True
    for platform_name in sorted(platforms.keys()):
        platform_upper = platform_name.upper().replace('-', '_')

        if first:
            header += f"#if defined(ALLOY_PLATFORM_{platform_upper})\n"
            first = False
        else:
            header += f"#elif defined(ALLOY_PLATFORM_{platform_upper})\n"

        header += f'    #include "platform/{platform_name}/{peripheral}.hpp"\n'

    # Add error for no platform defined
    header += """
#else
    #error "No platform defined! Please define ALLOY_PLATFORM_* in your build system."
    #error "Supported platforms: """

    platform_list = ', '.join([f"ALLOY_PLATFORM_{p.upper().replace('-', '_')}"
                               for p in sorted(platforms.keys())])
    header += platform_list
    header += '"\n'
    header += f"""    #error "Example CMake: target_compile_definitions(my_target PRIVATE ALLOY_PLATFORM_STM32F4)"
#endif

/**
 * After including this header, you can use the {peripheral_upper} implementation
 * for the selected platform without any code changes.
 *
 * Example:
 *   // Works on any platform with {peripheral_upper} support
 *   #include "hal/{peripheral}.hpp"
 *
 *   // Use {peripheral_upper} API (platform-specific implementation)
 *   // ... your code here ...
 */
"""

    return header


def main():
    """Main entry point"""
    print_header("Generating Universal HAL Wrapper Headers")
    print()

    # Discover peripherals and platforms
    print_info("Discovering peripherals...")
    peripherals = discover_peripherals()
    print_success(f"Found {len(peripherals)} peripherals: {', '.join(sorted(peripherals))}")
    print()

    print_info("Discovering platforms...")
    platforms = discover_platforms_and_mcus()
    print_success(f"Found {len(platforms)} platforms: {', '.join(sorted(platforms.keys()))}")
    print()

    # Output directory
    repo_root = CODEGEN_DIR.parent.parent
    output_dir = repo_root / "src" / "hal"

    # Generate wrapper for each peripheral
    print_info("Generating wrapper headers...")
    generated = 0

    for peripheral in sorted(peripherals):
        output_file = output_dir / f"{peripheral}.hpp"

        # Generate content
        content = generate_wrapper_header(peripheral, platforms)

        # Write file
        output_file.write_text(content)

        print_success(f"✅ {peripheral}.hpp")
        generated += 1

    print()
    print_header("Generation Complete")
    print()
    print_success(f"✅ Generated {generated} wrapper headers in src/hal/")
    print()
    print_info("Usage in application:")
    print('  #include "hal/gpio.hpp"')
    print('  #include "hal/uart.hpp"')
    print()
    print_info("CMake will automatically define ALLOY_PLATFORM_* based on your board configuration")
    print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
