#!/usr/bin/env python3
"""
Signal Table Generator

Generates C++ signal routing tables from pin function databases.
Part of Phase 2: Signal Metadata Generation.

This generator creates specializations of PeripheralSignal templates
defined in src/hal/signals.hpp, populating them with platform-specific
pin mappings and alternate function numbers.

Usage:
    python generate_signal_tables.py --vendor atmel --device same70 --output src/hal/vendors/atmel/same70/signals.hpp

Output:
    - Per-peripheral signal structs (e.g., Usart0TxSignal, Spi0MosiSignal)
    - Pin compatibility arrays with alternate function numbers
    - Signal type enums and mappings
"""

import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple
from dataclasses import dataclass
from collections import defaultdict

# Add parent to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.vendors.atmel.same70_pin_functions import SAME70_PIN_FUNCTIONS, PinFunction


@dataclass
class PeripheralSignal:
    """Represents a peripheral signal (e.g., USART0 TX)"""
    peripheral_name: str  # e.g., "USART0"
    peripheral_type: str  # e.g., "USART", "SPI", "TWI"
    signal_type: str      # e.g., "TX", "RX", "MOSI", "MISO", "SCL", "SDA"
    pins: List[Tuple[str, str]]  # List of (pin_name, peripheral_select)


class SignalTableGenerator:
    """Generates signal routing tables from pin function databases"""

    def __init__(self, vendor: str, device: str):
        self.vendor = vendor
        self.device = device
        self.signals: Dict[str, PeripheralSignal] = {}

    def extract_signals_from_pin_functions(self, pin_functions: Dict[str, List[PinFunction]]):
        """
        Extract peripheral signals from pin function database.

        Groups pins by their peripheral signals to create signal→pins mappings.
        """
        # Dictionary: (peripheral_name, signal_type) → list of (pin, periph_select)
        signal_pins = defaultdict(list)

        for pin_name, functions in pin_functions.items():
            for func in functions:
                # Skip PIO (GPIO) functions
                if func.peripheral_type == "PIO":
                    continue

                # Extract peripheral name and signal type
                peripheral_name, signal_type = self._parse_function_name(
                    func.function_name, func.peripheral_type
                )

                if peripheral_name and signal_type:
                    key = (peripheral_name, func.peripheral_type, signal_type)
                    signal_pins[key].append((pin_name, func.peripheral))

        # Convert to PeripheralSignal objects
        for (periph_name, periph_type, signal_type), pins in signal_pins.items():
            signal_id = f"{periph_name}_{signal_type}"
            self.signals[signal_id] = PeripheralSignal(
                peripheral_name=periph_name,
                peripheral_type=periph_type,
                signal_type=signal_type,
                pins=sorted(pins)  # Sort for deterministic output
            )

    def _parse_function_name(self, function_name: str, peripheral_type: str) -> Tuple[str, str]:
        """
        Parse function name to extract peripheral instance and signal type.

        Examples:
            "URXD0" → ("USART0", "RX")
            "UTXD0" → ("USART0", "TX")
            "MISO0" → ("SPI0", "MISO")
            "TWD0"  → ("TWI0", "SDA")
            "TWCK0" → ("TWI0", "SCL")
        """
        fname = function_name.upper()

        # USART patterns
        if "URXD" in fname or "RXD" in fname:
            # Extract instance number (URXD0 → 0, RXD1 → 1)
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"USART{instance}", "RX")
        elif "UTXD" in fname or "TXD" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"USART{instance}", "TX")

        # UART patterns
        elif fname == "RXDA":
            return ("UART0", "RX")
        elif fname == "TXDA":
            return ("UART0", "TX")

        # SPI patterns
        elif "MISO" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"SPI{instance}", "MISO")
        elif "MOSI" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"SPI{instance}", "MOSI")
        elif "SPCK" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"SPI{instance}", "CLK")
        elif "NPCS" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"SPI{instance}", "CS")

        # TWI/I2C patterns
        elif fname.startswith("TWD"):
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"TWI{instance}", "SDA")
        elif fname.startswith("TWCK"):
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"TWI{instance}", "SCL")

        # PWM patterns
        elif "PWMH" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            channel = match.group(1) if match else "0"
            return (f"PWM{channel}", "HIGH")
        elif "PWML" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            channel = match.group(1) if match else "0"
            return (f"PWM{channel}", "LOW")

        # ADC/DAC patterns
        elif "AD" in fname and peripheral_type == "ADC":
            import re
            match = re.search(r'(\d+)', fname)
            channel = match.group(1) if match else "0"
            return (f"ADC{channel}", "IN")
        elif "DATRG" in fname:
            return ("DAC", "TRIGGER")
        elif "DAOUT" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            channel = match.group(1) if match else "0"
            return (f"DAC{channel}", "OUT")

        # Timer/Counter patterns
        elif "TIOA" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"TC{instance}", "TIOA")
        elif "TIOB" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"TC{instance}", "TIOB")
        elif "TCLK" in fname:
            import re
            match = re.search(r'(\d+)', fname)
            instance = match.group(1) if match else "0"
            return (f"TC{instance}", "CLK")

        # Unable to parse
        return (None, None)

    def _pin_name_to_id(self, pin_name: str) -> str:
        """Convert pin name to PinId enum value (PA0 → PinId::PA0)"""
        return f"PinId::{pin_name}"

    def _peripheral_select_to_af(self, periph_select: str) -> str:
        """Convert peripheral select to AlternateFunction enum (A → AlternateFunction::PERIPH_A)"""
        if periph_select == "PIO":
            return "AlternateFunction::PERIPH_A"  # Default, though PIO shouldn't be in signals
        return f"AlternateFunction::PERIPH_{periph_select}"

    def generate_cpp_header(self, output_path: Path):
        """Generate C++ header file with signal tables"""

        # Group signals by peripheral type for organization
        signals_by_type = defaultdict(list)
        for signal in self.signals.values():
            signals_by_type[signal.peripheral_type].append(signal)

        # Generate header content
        content = self._generate_header_prologue()

        # Generate signal definitions for each peripheral type
        for periph_type in sorted(signals_by_type.keys()):
            content += self._generate_peripheral_signals(periph_type, signals_by_type[periph_type])

        content += self._generate_header_epilogue()

        # Write to file
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content)
        print(f"Generated signal table: {output_path}")

    def _generate_header_prologue(self) -> str:
        """Generate file header and includes"""
        return f"""/**
 * @file {self.device}_signals.hpp
 * @brief Signal Routing Tables for {self.device.upper()}
 *
 * Auto-generated signal routing tables for peripheral signal validation.
 * Part of Phase 2: Signal Metadata Generation.
 *
 * Generated from: {self.device}_pin_functions.py
 * Generator: generate_signal_tables.py
 *
 * @note Part of modernize-peripheral-architecture OpenSpec change
 * @see openspec/changes/modernize-peripheral-architecture/specs/signal-routing/spec.md
 */

#pragma once

#include "hal/signals.hpp"

namespace alloy::hal::{self.vendor}::{self.device} {{

using namespace alloy::hal::signals;

// ============================================================================
// Peripheral Signal Specializations
// ============================================================================

"""

    def _generate_peripheral_signals(self, periph_type: str, signals: List[PeripheralSignal]) -> str:
        """Generate signal definitions for a peripheral type"""
        content = f"// {periph_type} Signals\n"
        content += f"// {'-' * 78}\n\n"

        for signal in sorted(signals, key=lambda s: (s.peripheral_name, s.signal_type)):
            content += self._generate_signal_struct(signal)
            content += "\n"

        return content

    def _generate_signal_struct(self, signal: PeripheralSignal) -> str:
        """Generate a single signal struct specialization"""

        # Create struct name (USART0_RX → Usart0RxSignal)
        struct_name = f"{signal.peripheral_name}{signal.signal_type}Signal"

        # Map signal type to SignalType enum
        signal_type_enum = self._map_signal_type(signal.signal_type)

        # Generate pin array
        pin_array = ",\n        ".join([
            f"PinDef{{{self._pin_name_to_id(pin)}, {self._peripheral_select_to_af(af)}}}"
            for pin, af in signal.pins
        ])

        return f"""/**
 * @brief {signal.peripheral_name} {signal.signal_type} signal
 * Compatible pins: {', '.join([pin for pin, _ in signal.pins])}
 */
struct {struct_name} {{
    static constexpr PeripheralId peripheral = PeripheralId::{signal.peripheral_name.upper()};
    static constexpr SignalType type = SignalType::{signal_type_enum};
    static constexpr std::array compatible_pins = {{
        {pin_array}
    }};
}};
"""

    def _map_signal_type(self, signal_type: str) -> str:
        """Map signal type string to SignalType enum value"""
        # Direct mappings
        if signal_type in ["TX", "RX", "CLK", "CS", "MOSI", "MISO", "SCL", "SDA", "TRIGGER"]:
            return signal_type

        # Custom mappings
        mapping = {
            "HIGH": "DATA",
            "LOW": "DATA",
            "IN": "DATA",
            "OUT": "DATA",
            "TIOA": "DATA",
            "TIOB": "DATA",
        }
        return mapping.get(signal_type, "CUSTOM")

    def _generate_header_epilogue(self) -> str:
        """Generate file footer"""
        return f"""
}}  // namespace alloy::hal::{self.vendor}::{self.device}
"""


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description='Generate signal routing tables')
    parser.add_argument('--vendor', required=True, help='Vendor name (e.g., atmel)')
    parser.add_argument('--device', required=True, help='Device name (e.g., same70)')
    parser.add_argument('--output', required=True, help='Output header file path')

    args = parser.parse_args()

    # Create generator
    generator = SignalTableGenerator(args.vendor, args.device)

    # Load pin functions (currently only SAME70 supported)
    if args.vendor == "atmel" and args.device == "same70":
        generator.extract_signals_from_pin_functions(SAME70_PIN_FUNCTIONS)
    else:
        print(f"Error: Unsupported vendor/device: {args.vendor}/{args.device}")
        sys.exit(1)

    # Generate output
    output_path = Path(args.output)
    generator.generate_cpp_header(output_path)

    print(f"\nGenerated {len(generator.signals)} peripheral signals")
    print("Signal table generation complete!")


if __name__ == "__main__":
    main()
