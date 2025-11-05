"""
Code generation command for Alloy CLI

This command generates HAL code for MCUs from SVD files.
"""

import sys
from pathlib import Path

# Add parent directory to path to import existing modules
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, logger


def setup_parser(parser):
    """Setup the codegen command parser"""
    parser.add_argument(
        '--vendor',
        type=str,
        choices=['st', 'atmel', 'microchip', 'raspberrypi', 'espressif', 'all'],
        default='st',
        help='MCU vendor to generate code for (default: st)'
    )

    parser.add_argument(
        '--family',
        type=str,
        help='Specific MCU family to generate (e.g., stm32f4, sam3x)'
    )

    parser.add_argument(
        '--all',
        action='store_true',
        help='Generate code for all available families'
    )

    parser.add_argument(
        '--output',
        type=Path,
        help='Output directory (default: src/hal/vendors)'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be generated without actually generating'
    )


def execute(args):
    """Execute the codegen command"""
    print_header(f"🚀 Alloy Code Generation - {args.vendor.upper()}", "=")

    if args.dry_run:
        print_info("Running in dry-run mode (no files will be generated)")

    # Determine which vendor to process
    if args.vendor == 'st' or args.vendor == 'all':
        result = generate_st(args)
        if result != 0:
            return result

    if args.vendor == 'atmel' or args.vendor == 'all':
        result = generate_atmel(args)
        if result != 0:
            return result

    if args.vendor == 'microchip' or args.vendor == 'all':
        result = generate_microchip(args)
        if result != 0:
            return result

    if args.vendor == 'raspberrypi' or args.vendor == 'all':
        result = generate_raspberrypi(args)
        if result != 0:
            return result

    if args.vendor == 'espressif' or args.vendor == 'all':
        result = generate_espressif(args)
        if result != 0:
            return result

    print_success("Code generation completed successfully!")
    return 0


def generate_st(args):
    """Generate code for STMicroelectronics MCUs"""
    try:
        from cli.vendors.st.generate_all_st_pins import main as generate_st_main

        logger.info("Generating code for STMicroelectronics...")

        if args.dry_run:
            print_info("Would generate ST MCU code")
            return 0

        # Call the existing ST generator
        return generate_st_main()

    except ImportError as e:
        print_error(f"Failed to import ST generator: {e}")
        return 1
    except Exception as e:
        print_error(f"ST code generation failed: {e}")
        return 1


def generate_atmel(args):
    """Generate code for Atmel/Microchip MCUs"""
    try:
        from cli.vendors.atmel.generate_all_atmel import main as generate_atmel_main

        logger.info("Generating code for Atmel/Microchip...")

        if args.dry_run:
            print_info("Would generate Atmel/Microchip MCU code (SAME70, SAMV71, SAMD21)")
            return 0

        # Call the unified Atmel generator
        return generate_atmel_main()

    except ImportError as e:
        print_error(f"Failed to import Atmel generator: {e}")
        return 1
    except Exception as e:
        print_error(f"Atmel code generation failed: {e}")
        return 1


def generate_microchip(args):
    """Generate code for Microchip MCUs (PIC, dsPIC, etc.)"""
    try:
        logger.info("Generating code for Microchip...")

        if args.dry_run:
            print_info("Would generate Microchip MCU code")
            return 0

        # TODO: Implement Microchip generator
        print_info("Microchip code generation not yet implemented")
        return 0

    except Exception as e:
        print_error(f"Microchip code generation failed: {e}")
        return 1


def generate_raspberrypi(args):
    """Generate code for Raspberry Pi MCUs"""
    try:
        from cli.vendors.raspberrypi.generate_rp2040_pins import main as generate_rp2040_main

        logger.info("Generating code for Raspberry Pi...")

        if args.dry_run:
            print_info("Would generate Raspberry Pi RP2040 MCU code")
            return 0

        # Call the RP2040 generator
        return generate_rp2040_main()

    except ImportError as e:
        print_error(f"Failed to import Raspberry Pi generator: {e}")
        return 1
    except Exception as e:
        print_error(f"Raspberry Pi code generation failed: {e}")
        return 1


def generate_espressif(args):
    """Generate code for Espressif ESP32 MCUs"""
    try:
        from cli.vendors.espressif.generate_esp32_pins import main as generate_esp32_main

        logger.info("Generating code for Espressif ESP32 family...")

        if args.dry_run:
            print_info("Would generate Espressif ESP32 pin definitions and GPIO Matrix helpers")
            return 0

        # Call the ESP32 generator
        result = generate_esp32_main()

        if result == 0:
            print_info("")
            print_info("Peripheral register definitions already generated from SVD files:")
            print_info("  - ESP32 (original, dual-core Xtensa LX6)")
            print_info("  - ESP32-S2 (single-core Xtensa LX7, USB)")
            print_info("  - ESP32-S3 (dual-core Xtensa LX7, USB, AI)")
            print_info("  - ESP32-C2 (single-core RISC-V)")
            print_info("  - ESP32-C3 (single-core RISC-V, WiFi, BLE)")
            print_info("  - ESP32-C6 (single-core RISC-V, WiFi 6)")
            print_info("  - ESP32-H2 (single-core RISC-V, BLE, 802.15.4)")
            print_info("  - ESP32-P4 (dual-core RISC-V, high-performance)")

        return result

    except ImportError as e:
        print_error(f"Failed to import ESP32 generator: {e}")
        return 1
    except Exception as e:
        print_error(f"Espressif code generation failed: {e}")
        return 1
