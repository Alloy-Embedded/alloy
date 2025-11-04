#!/usr/bin/env python3
"""
Generate Pin Headers for ALL STMicroelectronics Families

This script discovers all available STM32 SVD files and generates
pin headers for all supported families.
"""

from pathlib import Path
import sys
from typing import Dict, List
from svd_discovery import discover_all_svds
from svd_pin_extractor import extract_mcu_info_from_svd

# Import the existing pin generation functions
from generate_pins_from_svd import (
    generate_pins_header,
    generate_traits_header,
    generate_hardware_header,
    extract_hardware_addresses,
    MCUInfo,
    MCUPackageInfo,
    OUTPUT_DIR
)

# Import pin function generator
from generate_pin_functions import (
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
            # New structure: hal/vendors/st/stm32f4/stm32f407vg (MCUs inside family folder)
            family_lower = family_name.lower().replace("xx", "")
            # Map family SVD names to consistent family folders
            # STM32F7x2 -> stm32f7, STM32F745 -> stm32f7, STM32F407 -> stm32f4, STM32F103 -> stm32f1
            if family_lower.startswith("stm32f0"):
                family_folder = "stm32f0"
            elif family_lower.startswith("stm32f1"):
                family_folder = "stm32f1"
            elif family_lower.startswith("stm32f2"):
                family_folder = "stm32f2"
            elif family_lower.startswith("stm32f3"):
                family_folder = "stm32f3"
            elif family_lower.startswith("stm32f4"):
                family_folder = "stm32f4"
            elif family_lower.startswith("stm32f7"):
                family_folder = "stm32f7"
            elif family_lower.startswith("stm32g0"):
                family_folder = "stm32g0"
            elif family_lower.startswith("stm32g4"):
                family_folder = "stm32g4"
            elif family_lower.startswith("stm32h7"):
                family_folder = "stm32h7"
            elif family_lower.startswith("stm32l0"):
                family_folder = "stm32l0"
            elif family_lower.startswith("stm32l1"):
                family_folder = "stm32l1"
            elif family_lower.startswith("stm32l4"):
                family_folder = "stm32l4"
            elif family_lower.startswith("stm32l5"):
                family_folder = "stm32l5"
            elif family_lower.startswith("stm32u5"):
                family_folder = "stm32u5"
            else:
                family_folder = family_lower

            device_dir = OUTPUT_DIR / "vendors" / "st" / family_folder / variant_name.lower()
            device_dir.mkdir(parents=True, exist_ok=True)

            # Generate pins.hpp
            pins_content = generate_pins_header(base_mcu, package)
            (device_dir / "pins.hpp").write_text(pins_content)

            # Generate traits.hpp
            traits_content = generate_traits_header(base_mcu, package)
            (device_dir / "traits.hpp").write_text(traits_content)

            # Generate hardware.hpp
            hardware_content = generate_hardware_header(base_mcu, addresses)
            (device_dir / "hardware.hpp").write_text(hardware_content)

            # Generate pin_functions.hpp
            get_pin_funcs_fn, _, family_arch = load_pin_database(family_name)
            available_pins = []
            for port_letter in sorted(var_config['ports'].keys()):
                for pin_num in var_config['ports'][port_letter]:
                    available_pins.append(f"P{port_letter}{pin_num}")

            pin_funcs_content = generate_pin_functions_header(
                variant_name, family_lower, available_pins,
                get_pin_funcs_fn, family_arch
            )
            (device_dir / "pin_functions.hpp").write_text(pin_funcs_content)

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
            (device_dir / "gpio.hpp").write_text(gpio_content)

            print(f"     ✓ Generated {package.get_gpio_pin_count()} GPIO pins")
            success_count += 1

        except Exception as e:
            print(f"     ✗ Error: {e}")
            import traceback
            traceback.print_exc()
            fail_count += 1

    return success_count, fail_count


def main():
    print("="*80)
    print("🚀 STM32 Multi-Family Pin Generator")
    print("="*80)
    print("Generating pin headers for all ST microcontroller families...\n")

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
