#!/usr/bin/env python3
"""
Generate Pin Functions Headers

Generates pin_functions.hpp files with alternate function mappings
for all STM32 MCU families.
"""

from pathlib import Path
import sys
from typing import Dict, List, Callable, Any

REPO_ROOT = Path(__file__).parent.parent.parent.parent
OUTPUT_DIR = REPO_ROOT / "src" / "hal"


# Family-specific database loaders
def load_pin_database(family: str):
    """Dynamically load pin function database for a specific family"""
    family_lower = family.lower()

    try:
        if family_lower.startswith("stm32f1"):
            from cli.vendors.st.stm32f1_pin_functions import get_pin_functions, PinFunction
            return get_pin_functions, PinFunction, "STM32F1"
        elif family_lower.startswith("stm32f4"):
            from cli.vendors.st.stm32f4_pin_functions import get_pin_functions, PinFunction
            return get_pin_functions, PinFunction, "STM32F4"
        elif family_lower.startswith("stm32f7"):
            from cli.vendors.st.stm32f7_pin_functions import get_pin_functions, PinFunction
            return get_pin_functions, PinFunction, "STM32F7"
        else:
            print(f"    ⚠️  Warning: No pin function database for family {family}, generating basic header")
            return None, None, None
    except ImportError as e:
        print(f"    ⚠️  Warning: Could not load pin database for {family}: {e}")
        return None, None, None


def generate_pin_functions_header(device_name: str, family: str, available_pins: List[str],
                                  get_pin_funcs_fn=None, family_arch: str = None) -> str:
    """Generate pin_functions.hpp content with alternate function data"""

    content = f"""#pragma once

#include <cstdint>
#include "pins.hpp"

namespace alloy::hal::{family}::{device_name.lower()}::pin_functions {{

// ============================================================================
// Pin Alternate Functions for {device_name.upper()}
// Auto-generated from {family_arch or 'STM32'} datasheet
// ============================================================================

"""

    # Only generate pin function mappings if we have a database
    if get_pin_funcs_fn is None:
        content += f"// No pin function database available for {family}\n"
        content += f"// Add pin function database in tools/codegen/{family.lower()}_pin_functions.py\n\n"
        content += f"}}  // namespace alloy::hal::{family}::{device_name.lower()}::pin_functions\n"
        return content

    # For STM32F1, add mode/config enums
    if family_arch == "STM32F1":
        content += """// Pin function types (STM32F1 specific)
enum class PinMode : uint8_t {
    Input = 0,
    Output_10MHz = 1,
    Output_2MHz = 2,
    Output_50MHz = 3,
};

enum class PinConfig : uint8_t {
    // Input modes (when PinMode = Input)
    Analog = 0,
    Floating = 1,
    PullUpDown = 2,

    // Output modes (when PinMode = Output_*)
    PushPull = 0,
    OpenDrain = 1,
    AltFunctionPushPull = 2,
    AltFunctionOpenDrain = 3,
};

"""

    # Group functions by peripheral type
    peripherals_by_type: Dict[str, List[tuple]] = {}

    for pin_name in sorted(available_pins):
        functions = get_pin_funcs_fn(pin_name)
        for func in functions:
            # Handle both STM32F1 (func.peripheral) and STM32F4 (func.peripheral_type)
            peripheral_type = getattr(func, 'peripheral', None) or getattr(func, 'peripheral_type', 'UNKNOWN')
            if peripheral_type not in peripherals_by_type:
                peripherals_by_type[peripheral_type] = []
            peripherals_by_type[peripheral_type].append((pin_name, func))

    # Generate peripheral-specific sections
    for peripheral_type in sorted(peripherals_by_type.keys()):
        content += f"\n// {peripheral_type} Functions\n"
        content += f"namespace {peripheral_type.lower()} {{\n\n"

        # Group by function name
        functions_map: Dict[str, List[tuple]] = {}  # function_name -> [(pin_name, af_number), ...]
        for pin_name, func in peripherals_by_type[peripheral_type]:
            # Handle both STM32F1 (func.name) and STM32F4 (func.function_name)
            func_name = getattr(func, 'name', None) or getattr(func, 'function_name', 'UNKNOWN')
            af_number = getattr(func, 'af_number', 0)  # STM32F4 has AF numbers

            if func_name not in functions_map:
                functions_map[func_name] = []
            functions_map[func_name].append((pin_name, af_number))

        # Generate constexpr for each function
        for func_name in sorted(functions_map.keys()):
            pins_info = functions_map[func_name]
            # Clean function name for C++ identifier
            cpp_name = func_name.replace("-", "_").replace(".", "_")

            if len(pins_info) == 1:
                pin_const = f"pins::{pins_info[0][0]}"
                content += f"    constexpr uint8_t {cpp_name:<30} = {pin_const};\n"
            else:
                # Multiple pins (due to remapping or alternate locations)
                pins_list = [p[0] for p in pins_info]
                content += f"    // {cpp_name} available on: {', '.join(pins_list)}\n"
                for i, (pin, af_num) in enumerate(pins_info):
                    pin_const = f"pins::{pin}"
                    suffix = f"_REMAP{i}" if i > 0 else ""
                    content += f"    constexpr uint8_t {cpp_name}{suffix:<26} = {pin_const};\n"

        content += f"\n}}  // namespace {peripheral_type.lower()}\n"

    # Add peripheral pin configuration helpers (only for STM32F1 which has PinMode/PinConfig)
    if family_arch == "STM32F1":
        content += """
// ============================================================================
// Pin Configuration Helpers (STM32F1 specific)
// ============================================================================

// ADC pin configuration helper
template<uint8_t Pin>
struct ADCPinConfig {
    static constexpr PinMode mode = PinMode::Input;
    static constexpr PinConfig config = PinConfig::Analog;
};

// PWM (Timer) output pin configuration helper
template<uint8_t Pin>
struct PWMPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

// UART TX pin configuration helper
template<uint8_t Pin>
struct UARTTxPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

// UART RX pin configuration helper
template<uint8_t Pin>
struct UARTRxPinConfig {
    static constexpr PinMode mode = PinMode::Input;
    static constexpr PinConfig config = PinConfig::Floating;
};

// I2C pin configuration helper (both SCL and SDA)
template<uint8_t Pin>
struct I2CPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionOpenDrain;
};

// SPI Master pin configuration helpers
template<uint8_t Pin>
struct SPIMOSIPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

template<uint8_t Pin>
struct SPIMISOPinConfig {
    static constexpr PinMode mode = PinMode::Input;
    static constexpr PinConfig config = PinConfig::Floating;
};

template<uint8_t Pin>
struct SPISCKPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

"""

    content += f"}}  // namespace alloy::hal::{family}::{device_name.lower()}::pin_functions\n"

    return content


def generate_for_variant(variant_name: str, family: str, package_type: str, available_ports: Dict[str, List[int]]) -> bool:
    """Generate pin_functions.hpp for a specific variant"""

    try:
        print(f"  Generating pin functions for {variant_name}...")

        # Load pin database for this family
        get_pin_funcs_fn, PinFunction, family_arch = load_pin_database(family)

        # Build list of available pins
        available_pins = []
        for port_letter in sorted(available_ports.keys()):
            for pin_num in available_ports[port_letter]:
                pin_name = f"P{port_letter}{pin_num}"
                available_pins.append(pin_name)

        # Generate header content
        content = generate_pin_functions_header(variant_name, family, available_pins,
                                                get_pin_funcs_fn, family_arch)

        # Determine output path based on family
        # STM32F103 -> st/stm32f103/generated/...
        # STM32F407 -> st/stm32f407/generated/...
        family_path = family.lower().replace("xx", "")  # STM32F103xx -> stm32f103
        device_dir = OUTPUT_DIR / "st" / family_path / "generated" / variant_name.lower()
        device_dir.mkdir(parents=True, exist_ok=True)

        output_file = device_dir / "pin_functions.hpp"
        output_file.write_text(content)

        print(f"    ✓ Generated pin_functions.hpp")
        return True

    except Exception as e:
        print(f"    ✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    print("🔧 Generating pin function headers for all ST families...\n")

    # All STM32 variants with their packages
    # Format: variant_name -> (family, package, ports)
    variants = {
        # STM32F1 Family
        'STM32F103C8': {
            'family': 'STM32F103',
            'package': 'LQFP48',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': [13, 14, 15],
                'D': [0, 1],
            }
        },
        'STM32F103CB': {
            'family': 'STM32F103',
            'package': 'LQFP48',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': [13, 14, 15],
                'D': [0, 1],
            }
        },
        'STM32F103RE': {
            'family': 'STM32F103',
            'package': 'LQFP64',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': list(range(16)),
                'D': [0, 1, 2],
                'E': list(range(16)),
            }
        },

        # STM32F4 Family
        'STM32F407VG': {
            'family': 'STM32F407',
            'package': 'LQFP100',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': list(range(16)),
                'D': list(range(16)),
                'E': list(range(16)),
            }
        },
        'STM32F407ZG': {
            'family': 'STM32F407',
            'package': 'LQFP144',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': list(range(16)),
                'D': list(range(16)),
                'E': list(range(16)),
                'F': list(range(16)),
                'G': list(range(16)),
            }
        },
        'STM32F405RG': {
            'family': 'STM32F405',
            'package': 'LQFP64',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': list(range(16)),
                'D': [0, 1, 2],
            }
        },
        'STM32F401CC': {
            'family': 'STM32F401',
            'package': 'LQFP48',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': [13, 14, 15],
            }
        },
        'STM32F411CE': {
            'family': 'STM32F411',
            'package': 'LQFP48',
            'ports': {
                'A': list(range(16)),
                'B': list(range(16)),
                'C': [13, 14, 15],
            }
        },
    }

    success_count = 0
    for variant_name, config in sorted(variants.items()):
        if generate_for_variant(variant_name, config['family'], config['package'], config['ports']):
            success_count += 1

    print(f"\n✅ Generated pin functions for {success_count}/{len(variants)} variants")
    return 0 if success_count == len(variants) else 1


if __name__ == "__main__":
    sys.exit(main())
