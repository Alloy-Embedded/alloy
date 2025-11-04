"""
Vendors command for Alloy CLI

This command lists supported MCU vendors and their families.
"""

from cli.core.logger import print_header, COLORS, ICONS


# Vendor information
VENDORS = {
    'st': {
        'name': 'STMicroelectronics',
        'status': '✅ Supported',
        'families': [
            'STM32F0 - Cortex-M0 (Entry Level)',
            'STM32F1 - Cortex-M3 (Mainstream)',
            'STM32F2 - Cortex-M3 (High Performance)',
            'STM32F3 - Cortex-M4 (Mixed Signal)',
            'STM32F4 - Cortex-M4F (High Performance)',
            'STM32F7 - Cortex-M7F (Very High Performance)',
            'STM32G0 - Cortex-M0+ (Entry Level)',
            'STM32G4 - Cortex-M4F (Mixed Signal)',
            'STM32H7 - Cortex-M7F (Ultra High Performance)',
            'STM32L0 - Cortex-M0+ (Ultra Low Power)',
            'STM32L1 - Cortex-M3 (Ultra Low Power)',
            'STM32L4 - Cortex-M4F (Ultra Low Power)',
            'STM32L5 - Cortex-M33 (Ultra Low Power + Security)',
            'STM32U5 - Cortex-M33 (Ultra Low Power)',
            'STM32WB - Cortex-M4F (Wireless)',
            'STM32WL - Cortex-M4F (Wireless)',
        ]
    },
    'atmel': {
        'name': 'Atmel/Microchip',
        'status': '✅ Supported',
        'families': [
            'SAME70 - Cortex-M7F (High Performance)',
            'SAMV71 - Cortex-M7F (Planned)',
            'SAM3X - Cortex-M3 (Arduino Due) (Planned)',
            'SAM4S - Cortex-M4 (Planned)',
            'SAMD21 - Cortex-M0+ (Arduino Zero) (Planned)',
            'SAMD51 - Cortex-M4F (Planned)',
        ]
    },
    'microchip': {
        'name': 'Microchip (PIC/dsPIC)',
        'status': '📋 Future',
        'families': [
            'PIC32MX - MIPS M4K',
            'PIC32MZ - MIPS microAptiv',
            'dsPIC33 - 16-bit DSP',
        ]
    },
    'nordic': {
        'name': 'Nordic Semiconductor',
        'status': '📋 Future',
        'families': [
            'nRF52 - Cortex-M4F (BLE)',
            'nRF53 - Cortex-M33 (BLE)',
            'nRF91 - Cortex-M33 (LTE-M/NB-IoT)',
        ]
    },
    'espressif': {
        'name': 'Espressif',
        'status': '📋 Future',
        'families': [
            'ESP32 - Xtensa LX6 (WiFi/BLE)',
            'ESP32-S2 - Xtensa LX7 (WiFi)',
            'ESP32-C3 - RISC-V (WiFi/BLE)',
        ]
    },
}


def setup_parser(parser):
    """Setup the vendors command parser"""
    parser.add_argument(
        '--detailed',
        action='store_true',
        help='Show detailed information for each vendor'
    )


def execute(args):
    """Execute the vendors command"""
    print_header("🏭 Supported MCU Vendors", "=")

    for vendor_id, vendor_info in VENDORS.items():
        # Print vendor name and status
        status_color = COLORS['GREEN'] if '✅' in vendor_info['status'] else COLORS['YELLOW']
        print(f"\n{COLORS['BOLD']}{COLORS['CYAN']}{vendor_info['name']}{COLORS['RESET']}")
        print(f"  Status: {status_color}{vendor_info['status']}{COLORS['RESET']}")
        print(f"  ID: {vendor_id}")

        if args.detailed or '✅' in vendor_info['status']:
            print(f"\n  {COLORS['BOLD']}Families:{COLORS['RESET']}")
            for family in vendor_info['families']:
                print(f"    • {family}")

    print(f"\n{COLORS['GRAY']}{'─' * 80}{COLORS['RESET']}")
    print(f"\n{ICONS['INFO']} Use 'alloy codegen --vendor <vendor>' to generate code")
    print(f"{ICONS['INFO']} Use 'alloy status' to see implementation status\n")

    return 0
