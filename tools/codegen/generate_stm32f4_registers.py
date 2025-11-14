#!/usr/bin/env python3
"""
Generate STM32F4 register definitions from CMSIS-SVD.
Creates registers and bitfields for the STM32F4 family.
"""

import sys
from pathlib import Path

# Add codegen to path
CODEGEN_DIR = Path(__file__).parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.generators.generate_registers import generate_for_device
from cli.core.logger import print_header, print_success, print_error

def main():
    print_header("Generating STM32F4 Register Definitions")

    # Find STM32F401 SVD file (representative of F4 family)
    svd_path = CODEGEN_DIR / "svd" / "upstream" / "cmsis-svd-data" / "data" / "STMicro" / "STM32F401.svd"

    if not svd_path.exists():
        print_error(f"SVD file not found: {svd_path}")
        return 1

    print(f"Using SVD: {svd_path}")

    # Generate at family level (stm32f4, not stm32f401)
    success = generate_for_device(svd_path, family_level=True, tracker=None)

    if success:
        print_success("Successfully generated STM32F4 register definitions!")
        print()
        print("Generated files:")
        print("  • src/hal/vendors/st/stm32f4/registers/*.hpp")
        print("  • src/hal/vendors/st/stm32f4/bitfields/*.hpp")
        return 0
    else:
        print_error("Failed to generate register definitions")
        return 1

if __name__ == "__main__":
    sys.exit(main())
