#!/usr/bin/env python3
"""
Temporary script to generate STM32G0 register definitions.
This will be integrated into the main codegen pipeline once STM32G0 pins are added.
"""

import sys
from pathlib import Path

# Add codegen to path
CODEGEN_DIR = Path(__file__).parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.generators.generate_registers import generate_for_device
from cli.core.logger import print_header, print_success, print_error

def main():
    print_header("Generating STM32G0 Register Definitions")

    # Find STM32G0B1 SVD file
    svd_path = CODEGEN_DIR / "svd" / "upstream" / "cmsis-svd-data" / "data" / "STMicro" / "STM32G0B1.svd"

    if not svd_path.exists():
        print_error(f"SVD file not found: {svd_path}")
        return 1

    print(f"Using SVD: {svd_path}")

    # Generate at family level (stm32g0, not stm32g0b1)
    success = generate_for_device(svd_path, family_level=True, tracker=None)

    if success:
        print_success("Successfully generated STM32G0 register definitions!")
        print()
        print("Generated files:")
        print("  • src/hal/vendors/st/stm32g0/registers/*.hpp")
        print("  • src/hal/vendors/st/stm32g0/bitfields/*.hpp")
        return 0
    else:
        print_error("Failed to generate register definitions")
        return 1

if __name__ == "__main__":
    sys.exit(main())
