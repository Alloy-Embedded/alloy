#!/usr/bin/env python3
"""
Generate STM32F7 register definitions from CMSIS-SVD.
Creates registers and bitfields for the STM32F7 family.
"""

import sys
from pathlib import Path

# Add codegen to path
CODEGEN_DIR = Path(__file__).parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.generators.generate_registers import generate_for_device
from cli.core.logger import print_header, print_success, print_error

def main():
    print_header("Generating STM32F7 Register Definitions")

    # Find STM32F7x2 SVD file (covers STM32F722/F723 family)
    svd_path = CODEGEN_DIR / "svd" / "upstream" / "cmsis-svd-data" / "data" / "STMicro" / "STM32F7x2.svd"

    if not svd_path.exists():
        print_error(f"SVD file not found: {svd_path}")
        return 1

    # Generate registers and bitfields for STM32F7 family
    success = generate_for_device(svd_path, family_level=True, tracker=None)

    if success:
        print_success("STM32F7 register generation complete!")
        print("\n📁 Generated files:")
        print("   - src/hal/vendors/st/stm32f7/registers/*.hpp")
        print("   - src/hal/vendors/st/stm32f7/bitfields/*.hpp")
        return 0
    else:
        print_error("STM32F7 register generation failed!")
        return 1

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
