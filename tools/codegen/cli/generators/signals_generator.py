#!/usr/bin/env python3
"""
Signal Routing Generator for SAME70 Family

Generates MCU-specific signals.hpp files from pin function database.
Similar to hardware_policy_generator.py and startup_generator.py.

Usage:
    ./signals_generator.py <mcu_name>        # Generate for specific MCU
    ./signals_generator.py --all             # Generate for all MCUs
    ./signals_generator.py --list            # List available MCUs

Example:
    ./signals_generator.py atsame70q21b
    ./signals_generator.py --all
"""

import json
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Set
from collections import defaultdict
from jinja2 import Environment, FileSystemLoader

# Add parent to path to import pin functions
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.vendors.atmel.same70_pin_functions import SAME70_PIN_FUNCTIONS


class SignalsGenerator:
    """
    Generator for SAME70 signal routing tables.

    Generates MCU-specific signals.hpp files by filtering the complete
    pin function database based on available pins for each MCU package.
    """

    def __init__(self, config_file: Path = None):
        if config_file is None:
            config_file = CODEGEN_DIR / "cli/generators/metadata/platform/same70_signal_config.json"

        with open(config_file, 'r') as f:
            self.config = json.load(f)

        template_dir = CODEGEN_DIR / "cli/generators/templates"
        self.env = Environment(loader=FileSystemLoader(str(template_dir)))
        self.template = self.env.get_template('signals.hpp.j2')

    def get_mcu_config(self, mcu_name: str) -> Dict:
        """Get configuration for specific MCU"""
        for mcu in self.config['mcus']:
            if mcu['name'] == mcu_name:
                return mcu
        raise ValueError(f"MCU '{mcu_name}' not found in configuration")

    def is_pin_available(self, pin_name: str, mcu_config: Dict) -> bool:
        """Check if pin is available on this MCU package"""
        # Extract port and pin number (e.g., "PA8" -> "PA", 8)
        if len(pin_name) < 3:
            return False

        port = pin_name[:2]  # "PA", "PB", etc.
        try:
            pin_num = int(pin_name[2:])
        except ValueError:
            return False

        # Check if port is available
        if port not in mcu_config['available_ports']:
            return False

        # Check if pin number is within range
        max_pins = mcu_config['max_pins_per_port'].get(port, 0)
        return pin_num < max_pins

    def map_signal_type(self, function_name: str) -> str:
        """Map function name to signal type enum"""
        # Extract signal suffix (e.g., "UART0_RXD" -> "RXD")
        parts = function_name.split('_')
        if len(parts) < 2:
            return "DATA"

        suffix = parts[-1].upper()

        # Map common suffixes
        type_map = self.config.get('signal_type_mapping', {})
        return type_map.get(suffix, "DATA")

    def extract_peripheral_info(self, function_name: str, peripheral_type: str) -> Dict:
        """Extract peripheral ID and signal name from function"""
        # Examples:
        # "UART0_RXD" -> peripheral="UART0", signal="RX"
        # "PWMC0_PWMH0" -> peripheral="PWM0", signal="HIGH"
        # "TWD0" -> peripheral="I2C0" (TWI), signal="DATA"

        parts = function_name.split('_')

        # Handle special cases
        if peripheral_type == "TWI":
            # TWD0 or TWCK0 -> I2C0
            if "TWD" in function_name:
                peripheral_id = f"I2C{function_name[-1]}"
                signal_name = "DATA"
            elif "TWCK" in function_name:
                peripheral_id = f"I2C{function_name[-1]}"
                signal_name = "CLOCK"
            else:
                peripheral_id = "I2C0"
                signal_name = "DATA"
        elif peripheral_type == "UART":
            # RXDA -> UART0
            peripheral_id = "UART0"
            signal_name = "RX" if "RX" in function_name else "TX"
        elif peripheral_type == "PWM":
            # PWMC0_PWMH0 -> PWM0_HIGH
            if "PWMH" in function_name:
                signal_name = "HIGH"
            elif "PWML" in function_name:
                signal_name = "LOW"
            else:
                signal_name = "DATA"
            # Extract number from PWMC0
            peripheral_id = "PWM0"
        elif peripheral_type == "USART":
            # URXD0 -> USART0
            for i in range(10):
                if str(i) in function_name:
                    peripheral_id = f"USART{i}"
                    break
            else:
                peripheral_id = "USART0"

            if "RX" in function_name or "URX" in function_name:
                signal_name = "RX"
            elif "TX" in function_name or "UTX" in function_name:
                signal_name = "TX"
            elif "CK" in function_name:
                signal_name = "CLOCK"
            elif "RTS" in function_name:
                signal_name = "RTS"
            elif "CTS" in function_name:
                signal_name = "CTS"
            else:
                signal_name = "DATA"
        elif peripheral_type == "SPI":
            # SPI0_MISO -> SPI0
            for i in range(10):
                if f"{peripheral_type}{i}" in function_name:
                    peripheral_id = f"{peripheral_type}{i}"
                    break
            else:
                peripheral_id = f"{peripheral_type}0"

            if "MISO" in function_name:
                signal_name = "MISO"
            elif "MOSI" in function_name:
                signal_name = "MOSI"
            elif "SPCK" in function_name or "SCK" in function_name:
                signal_name = "CLOCK"
            elif "NPCS" in function_name or "CS" in function_name:
                signal_name = "CS"
            else:
                signal_name = "DATA"
        elif peripheral_type == "TC":
            # TC0_TIOA0 -> Timer0
            for i in range(12):
                if f"TC{i}" in function_name or f"TIOA{i}" in function_name or f"TIOB{i}" in function_name:
                    peripheral_id = f"Timer{i}"
                    break
            else:
                peripheral_id = "Timer0"

            if "TIOA" in function_name:
                signal_name = "TIOA"
            elif "TIOB" in function_name:
                signal_name = "TIOB"
            elif "TCLK" in function_name:
                signal_name = "CLOCK"
            else:
                signal_name = "DATA"
        elif peripheral_type == "ADC":
            peripheral_id = "ADC0"
            signal_name = "TRIGGER" if "TRIG" in function_name else "IN"
        elif peripheral_type == "DAC":
            peripheral_id = "DAC"
            signal_name = "TRIGGER" if "TRIG" in function_name else "OUT"
        else:
            # Default: use peripheral_type + number extraction
            peripheral_id = peripheral_type + "0"
            signal_name = "DATA"

        return {
            'peripheral_id': peripheral_id,
            'signal_name': signal_name
        }

    def generate_signals(self, mcu_name: str) -> str:
        """Generate signals.hpp for specific MCU"""
        mcu_config = self.get_mcu_config(mcu_name)

        # Filter pins available for this MCU
        available_pins = {
            pin: functions
            for pin, functions in SAME70_PIN_FUNCTIONS.items()
            if self.is_pin_available(pin, mcu_config)
        }

        # Build signal structures grouped by peripheral
        signals_by_peripheral = defaultdict(list)

        # Track unique signals (peripheral + signal name)
        seen_signals = set()

        for pin_name, functions in sorted(available_pins.items()):
            for func in functions:
                # Skip PIO (GPIO) functions
                if func.peripheral_type == "PIO":
                    continue

                # Extract peripheral info
                info = self.extract_peripheral_info(func.function_name, func.peripheral_type)
                peripheral_id = info['peripheral_id']
                signal_name = info['signal_name']

                # Create unique signal key
                signal_key = f"{peripheral_id}_{signal_name}"

                # Find or create signal entry
                signal_found = False
                for sig in signals_by_peripheral[func.peripheral_type]:
                    if sig['name'] == signal_key:
                        # Add this pin to existing signal
                        sig['pins'].append(pin_name)
                        sig['pin_details'].append({
                            'pin_id': pin_name,
                            'af': func.peripheral
                        })
                        signal_found = True
                        break

                if not signal_found:
                    # Create new signal
                    signals_by_peripheral[func.peripheral_type].append({
                        'name': signal_key,
                        'struct_name': f"{peripheral_id.upper()}{signal_name.upper()}Signal",
                        'peripheral_id': peripheral_id.upper(),
                        'signal_type': self.map_signal_type(func.function_name),
                        'pins': [pin_name],
                        'pin_details': [{
                            'pin_id': pin_name,
                            'af': func.peripheral
                        }]
                    })

        # Render template
        output = self.template.render(
            mcu_name=mcu_name,
            package=mcu_config['package'],
            pin_count=mcu_config['pin_count'],
            available_ports=mcu_config['available_ports'],
            generation_date=datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            signals_by_peripheral=dict(signals_by_peripheral)
        )

        return output

    def write_signals_file(self, mcu_name: str):
        """Generate and write signals.hpp for MCU"""
        output = self.generate_signals(mcu_name)

        # Determine output path - go up 3 levels from CODEGEN_DIR to reach project root
        project_root = CODEGEN_DIR.parent.parent
        output_dir = project_root / f"src/hal/vendors/atmel/same70/{mcu_name}"
        output_dir.mkdir(parents=True, exist_ok=True)

        output_file = output_dir / "signals.hpp"

        # Write file
        with open(output_file, 'w') as f:
            f.write(output)

        return output_file

    def generate_all(self):
        """Generate signals.hpp for all MCUs"""
        results = []
        for mcu in self.config['mcus']:
            mcu_name = mcu['name']
            print(f"🔨 Generating signals for {mcu_name}...")
            output_file = self.write_signals_file(mcu_name)
            results.append((mcu_name, output_file))
            print(f"✅ Generated: {output_file}")

        return results

    def list_mcus(self):
        """List all available MCUs"""
        print("📚 Available SAME70 MCUs:")
        print("=" * 60)
        for mcu in self.config['mcus']:
            print(f"  - {mcu['name']:20} {mcu['description']}")
            print(f"    Package: {mcu['package']}-pin ({mcu['pin_count']} pins)")
            print(f"    Ports: {', '.join(mcu['available_ports'])}")
            print()


def main():
    if len(sys.argv) < 2:
        print("Usage: ./signals_generator.py <mcu_name|--all|--list>")
        print("\nExamples:")
        print("  ./signals_generator.py atsame70q21b")
        print("  ./signals_generator.py --all")
        print("  ./signals_generator.py --list")
        sys.exit(1)

    generator = SignalsGenerator()

    if sys.argv[1] == "--list":
        generator.list_mcus()
    elif sys.argv[1] == "--all":
        print("🚀 Generating signals for all SAME70 MCUs...\n")
        results = generator.generate_all()
        print(f"\n✅ Successfully generated signals for {len(results)} MCUs!")
    else:
        mcu_name = sys.argv[1]
        print(f"🔨 Generating signals for {mcu_name}...")
        output_file = generator.write_signals_file(mcu_name)
        print(f"✅ Successfully generated: {output_file}")


if __name__ == "__main__":
    main()
