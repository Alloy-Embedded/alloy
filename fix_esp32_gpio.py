#!/usr/bin/env python3
"""
Fix ESP32 GPIO peripheral definition in espressif_esp32.json

This script replaces the incorrect GPIO peripheral (which has DPORT registers)
with the correct GPIO register structure based on ESP-IDF gpio_struct.h
"""

import json
import sys

# Correct GPIO peripheral definition based on ESP-IDF gpio_struct.h
# Reference: esp-idf/components/soc/esp32/register/soc/gpio_struct.h
CORRECT_GPIO_PERIPHERAL = {
    "GPIO": {
        "instances": [
            {
                "name": "GPIO",
                "base": "0x3FF44000",
                "irq": 22
            }
        ],
        "registers": {
            "BT_SELECT": {
                "offset": "0x00",
                "size": 32,
                "description": "NA"
            },
            "OUT": {
                "offset": "0x04",
                "size": 32,
                "description": "GPIO0~31 output value"
            },
            "OUT_W1TS": {
                "offset": "0x08",
                "size": 32,
                "description": "GPIO0~31 output value write 1 to set"
            },
            "OUT_W1TC": {
                "offset": "0x0C",
                "size": 32,
                "description": "GPIO0~31 output value write 1 to clear"
            },
            "OUT1": {
                "offset": "0x10",
                "size": 32,
                "description": "GPIO32~39 output value"
            },
            "OUT1_W1TS": {
                "offset": "0x14",
                "size": 32,
                "description": "GPIO32~39 output value write 1 to set"
            },
            "OUT1_W1TC": {
                "offset": "0x18",
                "size": 32,
                "description": "GPIO32~39 output value write 1 to clear"
            },
            "SDIO_SELECT": {
                "offset": "0x1C",
                "size": 32,
                "description": "SDIO PADS on/off control from outside"
            },
            "ENABLE": {
                "offset": "0x20",
                "size": 32,
                "description": "GPIO0~31 output enable"
            },
            "ENABLE_W1TS": {
                "offset": "0x24",
                "size": 32,
                "description": "GPIO0~31 output enable write 1 to set"
            },
            "ENABLE_W1TC": {
                "offset": "0x28",
                "size": 32,
                "description": "GPIO0~31 output enable write 1 to clear"
            },
            "ENABLE1": {
                "offset": "0x2C",
                "size": 32,
                "description": "GPIO32~39 output enable"
            },
            "ENABLE1_W1TS": {
                "offset": "0x30",
                "size": 32,
                "description": "GPIO32~39 output enable write 1 to set"
            },
            "ENABLE1_W1TC": {
                "offset": "0x34",
                "size": 32,
                "description": "GPIO32~39 output enable write 1 to clear"
            },
            "STRAP": {
                "offset": "0x38",
                "size": 32,
                "description": "GPIO strapping results"
            },
            "IN": {
                "offset": "0x3C",
                "size": 32,
                "description": "GPIO0~31 input value"
            },
            "IN1": {
                "offset": "0x40",
                "size": 32,
                "description": "GPIO32~39 input value"
            },
            "STATUS": {
                "offset": "0x44",
                "size": 32,
                "description": "GPIO0~31 interrupt status"
            },
            "STATUS_W1TS": {
                "offset": "0x48",
                "size": 32,
                "description": "GPIO0~31 interrupt status write 1 to set"
            },
            "STATUS_W1TC": {
                "offset": "0x4C",
                "size": 32,
                "description": "GPIO0~31 interrupt status write 1 to clear"
            },
            "STATUS1": {
                "offset": "0x50",
                "size": 32,
                "description": "GPIO32~39 interrupt status"
            },
            "STATUS1_W1TS": {
                "offset": "0x54",
                "size": 32,
                "description": "GPIO32~39 interrupt status write 1 to set"
            },
            "STATUS1_W1TC": {
                "offset": "0x58",
                "size": 32,
                "description": "GPIO32~39 interrupt status write 1 to clear"
            }
        },
        "bits": {}
    }
}

def fix_esp32_json(input_file, output_file=None):
    """Fix the ESP32 JSON file by replacing incorrect GPIO peripheral"""

    if output_file is None:
        output_file = input_file

    print(f"Reading {input_file}...")
    with open(input_file, 'r') as f:
        data = json.load(f)

    # Navigate to ESP32 MCU peripherals
    if 'mcus' not in data or 'ESP32' not in data['mcus']:
        print("ERROR: ESP32 MCU not found in JSON file")
        return False

    peripherals = data['mcus']['ESP32']['peripherals']

    # Check current GPIO peripheral
    if 'GPIO' in peripherals:
        old_gpio = peripherals['GPIO']
        old_instances = old_gpio.get('instances', [])
        old_registers = len(old_gpio.get('registers', {}))

        print(f"Found existing GPIO peripheral:")
        print(f"  - Instances: {len(old_instances)}")
        for inst in old_instances:
            print(f"    - {inst['name']} @ {inst['base']}")
        print(f"  - Registers: {old_registers}")

    # Replace with correct GPIO peripheral
    print("\nReplacing with correct GPIO peripheral...")
    peripherals['GPIO'] = CORRECT_GPIO_PERIPHERAL['GPIO']

    new_gpio = peripherals['GPIO']
    new_instances = new_gpio.get('instances', [])
    new_registers = len(new_gpio.get('registers', {}))

    print(f"New GPIO peripheral:")
    print(f"  - Instances: {len(new_instances)}")
    for inst in new_instances:
        print(f"    - {inst['name']} @ {inst['base']}")
    print(f"  - Registers: {new_registers}")

    # Write fixed JSON
    print(f"\nWriting {output_file}...")
    with open(output_file, 'w') as f:
        json.dump(data, f, indent=2)

    print("✓ ESP32 GPIO peripheral fixed successfully!")
    return True

if __name__ == '__main__':
    input_file = '/Users/lgili/Documents/01 - Codes/01 - Github/corezero/tools/codegen/database/families/espressif_esp32.json'

    if not fix_esp32_json(input_file):
        sys.exit(1)

    print("\nNext steps:")
    print("1. Regenerate ESP32 peripherals:")
    print("   python3 tools/codegen/generator.py --mcu ESP32 \\")
    print("       --database tools/codegen/database/families/espressif_esp32.json \\")
    print("       --output src/generated/espressif_systems_shanghai_co_ltd/esp32/esp32")
    print("2. Rebuild ESP32 blink example")
