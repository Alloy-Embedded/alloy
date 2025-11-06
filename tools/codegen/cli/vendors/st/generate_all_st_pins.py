#!/usr/bin/env python3
"""
Generate Pin Headers for ALL STMicroelectronics Families

This script discovers all available STM32 SVD files and generates
pin headers for all supported families.
"""

from pathlib import Path
import sys
from typing import Dict, List

# Import from parsers
from cli.parsers.svd_discovery import discover_all_svds
from cli.parsers.svd_pin_extractor import extract_mcu_info_from_svd

# Import path utilities and config
from cli.core.paths import get_mcu_output_dir, ensure_dir
from cli.core.config import detect_family, normalize_vendor

# Import progress tracking
from cli.core.progress import get_global_tracker

# Import the existing pin generation functions from same vendor folder
from cli.vendors.st.generate_pins_from_svd import (
    generate_pins_header,
    generate_traits_header,
    generate_hardware_header,
    extract_hardware_addresses,
    MCUInfo,
    MCUPackageInfo
)

# Import pin function generator from same vendor folder
from cli.vendors.st.generate_pin_functions import (
    generate_pin_functions_header,
    load_pin_database
)


# Family-specific variant configurations
# For families we don't have detailed pin configs, we use generic ones
ST_FAMILY_CONFIGS = {
    # STM32F0 - Entry level Cortex-M0
    "STM32F030": {
        "variants": {
            "STM32F030C6": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15]}},
            "STM32F030C8": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15]}},
        }
    },

    # STM32F1 - Mainstream Cortex-M3
    "STM32F103": {
        "variants": {
            "STM32F103C4": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15], "D": [0, 1]}},
            "STM32F103C6": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15], "D": [0, 1]}},
            "STM32F103C8": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15], "D": [0, 1]}},
            "STM32F103CB": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15], "D": [0, 1]}},
            "STM32F103RB": {"package": "LQFP64", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": [0, 1, 2]}},
            "STM32F103RC": {"package": "LQFP64", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": [0, 1, 2]}},
            "STM32F103RE": {"package": "LQFP64", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": [0, 1, 2], "E": list(range(16))}},
        }
    },

    # STM32F4 - High performance Cortex-M4
    "STM32F401": {
        "variants": {
            "STM32F401CC": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15]}},
            "STM32F401CE": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15]}},
        }
    },
    "STM32F405": {
        "variants": {
            "STM32F405RG": {"package": "LQFP64", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": [0, 1, 2]}},
        }
    },
    "STM32F407": {
        "variants": {
            "STM32F407VE": {"package": "LQFP100", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16))}},
            "STM32F407VG": {"package": "LQFP100", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16))}},
            "STM32F407ZG": {"package": "LQFP144", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16)), "F": list(range(16)), "G": list(range(16))}},
        }
    },
    "STM32F411": {
        "variants": {
            "STM32F411CE": {"package": "LQFP48", "ports": {"A": list(range(16)), "B": list(range(16)), "C": [13, 14, 15]}},
            "STM32F411RE": {"package": "LQFP64", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": [0, 1, 2]}},
        }
    },
    "STM32F429": {
        "variants": {
            "STM32F429ZI": {"package": "LQFP144", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16)), "F": list(range(16)), "G": list(range(16))}},
        }
    },

    # STM32F7 - High performance Cortex-M7
    "STM32F7x2": {  # F722, F732
        "variants": {
            "STM32F722RE": {"package": "LQFP64", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": [0, 1, 2]}},
            "STM32F722ZE": {"package": "LQFP144", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16)), "F": list(range(16)), "G": list(range(16))}},
        }
    },
    "STM32F745": {  # F745, F746
        "variants": {
            "STM32F745VG": {"package": "LQFP100", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16))}},
            "STM32F745ZG": {"package": "LQFP144", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16)), "F": list(range(16)), "G": list(range(16))}},
            "STM32F746VG": {"package": "LQFP100", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16))}},
            "STM32F746ZG": {"package": "LQFP144", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16)), "F": list(range(16)), "G": list(range(16))}},
        }
    },
    "STM32F765": {  # F765, F767, F769
        "variants": {
            "STM32F765VI": {"package": "LQFP100", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16))}},
            "STM32F767ZI": {"package": "LQFP144", "ports": {"A": list(range(16)), "B": list(range(16)), "C": list(range(16)), "D": list(range(16)), "E": list(range(16)), "F": list(range(16)), "G": list(range(16)), "H": [0, 1]}},
        }
    },
}


def generate_family(family_name: str, family_config: dict, svd_file: Path) -> tuple:
    """Generate all files for a specific ST family"""
    print(f"\n{'='*80}")
    print(f"📦 Family: {family_name}")
    print(f"{'='*80}")
    print(f"📄 SVD: {svd_file.name}\n")

    success_count = 0
    fail_count = 0

    for variant_name, var_config in sorted(family_config["variants"].items()):
        try:
            print(f"  🔨 Generating {variant_name} ({var_config['package']})...")

            # Extract base MCU info from SVD
            base_mcu = extract_mcu_info_from_svd(svd_file)

            # Apply variant configuration
            package = MCUPackageInfo(
                package_name=var_config['package'],
                total_pins=sum(len(pins) for pins in var_config['ports'].values()),
                available_ports=var_config['ports']
            )
            base_mcu.device_name = variant_name
            base_mcu.packages = [package]

            # Extract hardware addresses from SVD
            addresses = extract_hardware_addresses(svd_file)

            # Create output directory
            # Use centralized family detection from config.py
            family_folder = detect_family(variant_name, "st")
            vendor = normalize_vendor("STMicroelectronics")

            # Use centralized path management
            device_dir = ensure_dir(get_mcu_output_dir(vendor, family_folder, variant_name.lower()))

            # Get progress tracker if available
            tracker = get_global_tracker()
            if tracker:
                # Register all files we'll generate for this MCU
                expected_files = ["pins.hpp", "gpio.hpp", "hardware.hpp", "traits.hpp", "pin_functions.hpp"]
                tracker.add_mcu_task(vendor, family_folder, variant_name.lower(), expected_files)
                tracker.mark_mcu_generating(vendor, family_folder, variant_name.lower())

            # Generate pins.hpp
            if tracker:
                tracker.mark_file_generating(vendor, family_folder, variant_name.lower(), "pins.hpp")
            pins_content = generate_pins_header(base_mcu, package)
            pins_path = device_dir / "pins.hpp"
            pins_path.write_text(pins_content)
            if tracker:
                tracker.mark_file_success(vendor, family_folder, variant_name.lower(), "pins.hpp", pins_path)

            # Generate traits.hpp
            if tracker:
                tracker.mark_file_generating(vendor, family_folder, variant_name.lower(), "traits.hpp")
            traits_content = generate_traits_header(base_mcu, package)
            traits_path = device_dir / "traits.hpp"
            traits_path.write_text(traits_content)
            if tracker:
                tracker.mark_file_success(vendor, family_folder, variant_name.lower(), "traits.hpp", traits_path)

            # Generate hardware.hpp
            if tracker:
                tracker.mark_file_generating(vendor, family_folder, variant_name.lower(), "hardware.hpp")
            hardware_content = generate_hardware_header(base_mcu, addresses)
            hardware_path = device_dir / "hardware.hpp"
            hardware_path.write_text(hardware_content)
            if tracker:
                tracker.mark_file_success(vendor, family_folder, variant_name.lower(), "hardware.hpp", hardware_path)

            # Generate pin_functions.hpp
            get_pin_funcs_fn, _, family_arch = load_pin_database(family_name)
            available_pins = []
            for port_letter in sorted(var_config['ports'].keys()):
                for pin_num in var_config['ports'][port_letter]:
                    available_pins.append(f"P{port_letter}{pin_num}")

            if tracker:
                tracker.mark_file_generating(vendor, family_folder, variant_name.lower(), "pin_functions.hpp")
            pin_funcs_content = generate_pin_functions_header(
                variant_name, family_folder, available_pins,
                get_pin_funcs_fn, family_arch
            )
            pin_funcs_path = device_dir / "pin_functions.hpp"
            pin_funcs_path.write_text(pin_funcs_content)
            if tracker:
                tracker.mark_file_success(vendor, family_folder, variant_name.lower(), "pin_functions.hpp", pin_funcs_path)

            # Determine GPIO HAL path based on family architecture
            # New structure: MCUs are inside family folder, so gpio_hal.hpp is always ../gpio_hal.hpp
            # STM32F1/F0 use CRL/CRH registers -> have their own gpio_hal.hpp
            # STM32F4/F2/F7/L4/G4/H7/U5 use MODER/OSPEEDR registers -> can share gpio_hal.hpp
            if family_name.startswith("STM32F1") or family_name.startswith("STM32F0") or family_name.startswith("STM32F10"):
                gpio_hal_include = "../gpio_hal.hpp"
                gpio_namespace = family_folder
            else:
                # Use STM32F4 HAL for modern families (F4, F2, F7, L4, G4, H7, U5)
                # All are inside their family folders now, so always ../gpio_hal.hpp
                gpio_hal_include = "../gpio_hal.hpp"
                gpio_namespace = family_folder

            # Generate gpio.hpp (single-include)
            if tracker:
                tracker.mark_file_generating(vendor, family_folder, variant_name.lower(), "gpio.hpp")
            gpio_content = f"""#pragma once

// ============================================================================
// Single-include GPIO header for {variant_name}
// Include this file to get all GPIO functionality
//
// Usage:
//   #include \"hal/vendors/st/{family_folder}/{variant_name.lower()}/gpio.hpp\"
//
//   using namespace gpio;
//   using LED = GPIOPin<pins::PC13>;
//   LED::configureOutput();
//   LED::set();
// ============================================================================

#include "pins.hpp"
#include "pin_functions.hpp"
#include "traits.hpp"
#include "hardware.hpp"
#include "{gpio_hal_include}"

namespace alloy::hal::{family_folder}::{variant_name.lower()} {{

// Alias GPIO HAL template with this MCU's hardware
template<uint8_t Pin>
using GPIOPin = alloy::hal::{gpio_namespace}::GPIOPin<Hardware, Pin>;

}}  // namespace alloy::hal::{family_folder}::{variant_name.lower()}

// Convenience namespace alias
namespace gpio = alloy::hal::{family_folder}::{variant_name.lower()};
"""
            gpio_path = device_dir / "gpio.hpp"
            gpio_path.write_text(gpio_content)
            if tracker:
                tracker.mark_file_success(vendor, family_folder, variant_name.lower(), "gpio.hpp", gpio_path)
                tracker.complete_mcu_generation(vendor, family_folder, variant_name.lower(), True)

            print(f"     ✓ Generated {package.get_gpio_pin_count()} GPIO pins")
            success_count += 1

        except Exception as e:
            print(f"     ✗ Error: {e}")
            import traceback
            traceback.print_exc()

            # Mark as failed in tracker
            tracker = get_global_tracker()
            if tracker:
                tracker.complete_mcu_generation(vendor, family_folder, variant_name.lower(), False)

            fail_count += 1

    return success_count, fail_count


def main():
    print("="*80)
    print("🚀 STM32 Multi-Family Pin Generator")
    print("="*80)
    print("Generating pin headers for all ST microcontroller families...\n")

    # Set generator ID for manifest tracking
    tracker = get_global_tracker()
    if tracker:
        tracker.set_generator("st_pins")

    # Discover all available SVDs
    all_svds = discover_all_svds()

    total_success = 0
    total_fail = 0
    families_processed = 0

    # Generate for each configured family
    for family_name, family_config in ST_FAMILY_CONFIGS.items():
        # Find SVD file
        svd_key = f"{family_name}xx"  # STM32F103 -> STM32F103xx
        if svd_key not in all_svds:
            # Try without xx
            svd_key = family_name
            if svd_key not in all_svds:
                print(f"\n⚠️  No SVD found for family {family_name}")
                continue

        svd_file = all_svds[svd_key].file_path
        success, fail = generate_family(family_name, family_config, svd_file)
        total_success += success
        total_fail += fail
        families_processed += 1

    # Summary
    print(f"\n{'='*80}")
    print(f"📊 Summary")
    print(f"{'='*80}")
    print(f"✅ Successfully generated: {total_success} MCU variants")
    print(f"⚠️  Failed: {total_fail} variants")
    print(f"📦 Families processed: {families_processed}")
    print()

    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
