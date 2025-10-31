#!/usr/bin/env python3
"""
Generate All MCU Code

Batch processes all SVD files and generates code for all MCUs.
Output goes to src/generated/{vendor}/{family}/{mcu}/

Usage:
    python3 generate_all.py --vendor STMicro
    python3 generate_all.py --all
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List
import subprocess

SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent.parent
SVD_DIR = SCRIPT_DIR / "upstream" / "cmsis-svd-data" / "data"
OUTPUT_DIR = REPO_ROOT / "src" / "generated"
DATABASE_DIR = SCRIPT_DIR / "database" / "families"

class Colors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    INFO = '\033[96m'
    BOLD = '\033[1m'
    ENDC = '\033[0m'

def print_success(msg: str):
    print(f"{Colors.OKGREEN}✓{Colors.ENDC} {msg}")

def print_info(msg: str):
    print(f"{Colors.INFO}→{Colors.ENDC} {msg}")

def print_header(msg: str):
    print(f"\n{Colors.BOLD}{'='*60}{Colors.ENDC}")
    print(f"{Colors.BOLD}{msg}{Colors.ENDC}")
    print(f"{Colors.BOLD}{'='*60}{Colors.ENDC}\n")

def print_error(msg: str):
    print(f"{Colors.FAIL}✗{Colors.ENDC} {msg}")


def get_vendor_mapping(vendor_dir: str) -> Dict[str, str]:
    """Map vendor directory names to clean names"""
    mappings = {
        "STMicro": "st",
        "Nordic": "nordic",
        "NXP": "nxp",
        "Atmel": "atmel",
        "SiliconLabs": "silabs",
        "Toshiba": "toshiba",
        "Holtek": "holtek",
        "Nuvoton": "nuvoton"
    }
    return mappings.get(vendor_dir, vendor_dir.lower())


def get_family_from_mcu(mcu_name: str) -> str:
    """Extract family from MCU name"""
    mcu_lower = mcu_name.lower()

    # STM32 families
    if mcu_lower.startswith("stm32"):
        # STM32F103 -> stm32f1, STM32F446 -> stm32f4
        family_code = mcu_lower[5:7]  # Get F1, F4, L4, etc
        return f"stm32{family_code}"

    # nRF families
    if mcu_lower.startswith("nrf"):
        # nRF52840 -> nrf52
        return mcu_lower[:5]

    # Default: lowercase MCU name
    return mcu_lower


def generate_for_database(database_path: Path, vendor: str):
    """Generate code for all MCUs in a database"""

    print_info(f"Processing {database_path.name}...")

    # Load database
    with open(database_path) as f:
        db = json.load(f)

    vendor_clean = vendor
    if 'vendor' in db:
        vendor_clean = get_vendor_mapping(db['vendor'])

    family = db.get('family', 'unknown').lower()
    mcus = db.get('mcus', {})

    print_info(f"  Vendor: {vendor_clean}, Family: {family}, MCUs: {len(mcus)}")

    # Generate for each MCU
    for mcu_name in mcus.keys():
        mcu_lower = mcu_name.lower()
        mcu_output = OUTPUT_DIR / vendor_clean / family / mcu_lower

        print_info(f"    Generating {mcu_name}...")

        # Call generator
        cmd = [
            sys.executable,
            str(SCRIPT_DIR / "generator.py"),
            "--mcu", mcu_name,
            "--database", str(database_path),
            "--output", str(mcu_output)
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print_error(f"      Failed: {result.stderr}")
        else:
            print_success(f"      → {mcu_output.relative_to(REPO_ROOT)}")


def generate_vendor_readme(vendor_dir: Path, vendor_name: str):
    """Generate README for vendor directory"""

    # Count MCUs
    mcu_count = 0
    families = []

    for family_dir in vendor_dir.iterdir():
        if family_dir.is_dir():
            families.append(family_dir.name)
            mcu_dirs = [d for d in family_dir.iterdir() if d.is_dir()]
            mcu_count += len(mcu_dirs)

    readme_content = f"""# {vendor_name.upper()} Generated Code

**MCUs Supported**: {mcu_count}
**Families**: {', '.join(sorted(families))}

## Structure

```
{vendor_dir.name}/
"""

    for family in sorted(families):
        family_dir = vendor_dir / family
        mcus = sorted([d.name.upper() for d in family_dir.iterdir() if d.is_dir()])
        readme_content += f"├── {family}/\n"
        for mcu in mcus[:5]:  # Show first 5
            readme_content += f"│   ├── {mcu}/\n"
            readme_content += f"│   │   ├── startup.cpp\n"
            readme_content += f"│   │   └── peripherals.hpp\n"
        if len(mcus) > 5:
            readme_content += f"│   └── ... ({len(mcus) - 5} more)\n"

    readme_content += """```

## Files Generated

Each MCU directory contains:

- **startup.cpp** - Startup code with:
  - Reset handler (.data/.bss initialization)
  - Vector table (all interrupts)
  - Weak default handlers

- **peripherals.hpp** - Peripheral definitions:
  - Memory map (flash/RAM addresses)
  - All peripheral base addresses
  - Register structures
  - Peripheral instances (ready to use)

## Usage

```cpp
// In your CMakeLists.txt
set(ALLOY_MCU "STM32F103C8")  # Or any supported MCU

# Generated code is automatically included
target_sources(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}/startup.cpp
)
```

## Regenerating

To regenerate all code for this vendor:

```bash
cd tools/codegen
python3 generate_all.py --vendor {vendor_name}
```
"""

    readme_path = vendor_dir / "README.md"
    readme_path.write_text(readme_content)
    print_success(f"  Generated {readme_path.relative_to(REPO_ROOT)}")


def generate_index(output_dir: Path):
    """Generate master index of all MCUs"""

    vendors = {}
    total_mcus = 0

    for vendor_dir in output_dir.iterdir():
        if not vendor_dir.is_dir() or vendor_dir.name.startswith('.'):
            continue

        vendor_mcus = []
        for family_dir in vendor_dir.iterdir():
            if not family_dir.is_dir():
                continue
            for mcu_dir in family_dir.iterdir():
                if mcu_dir.is_dir():
                    vendor_mcus.append({
                        'name': mcu_dir.name.upper(),
                        'family': family_dir.name,
                        'path': str(mcu_dir.relative_to(output_dir))
                    })

        vendors[vendor_dir.name] = sorted(vendor_mcus, key=lambda x: x['name'])
        total_mcus += len(vendor_mcus)

    index_content = f"""# Alloy Generated MCU Support

**Total MCUs**: {total_mcus}
**Vendors**: {len(vendors)}

## Supported MCUs by Vendor

"""

    for vendor, mcus in sorted(vendors.items()):
        index_content += f"\n### {vendor.upper()} ({len(mcus)} MCUs)\n\n"

        # Group by family
        families = {}
        for mcu in mcus:
            family = mcu['family']
            if family not in families:
                families[family] = []
            families[family].append(mcu['name'])

        for family, mcu_names in sorted(families.items()):
            index_content += f"**{family.upper()}**: "
            index_content += ", ".join(mcu_names)
            index_content += "\n\n"

    index_content += """
## Directory Structure

```
src/generated/
├── {vendor}/
│   ├── {family}/
│   │   ├── {mcu}/
│   │   │   ├── startup.cpp
│   │   │   └── peripherals.hpp
│   │   └── ...
│   └── README.md
└── INDEX.md (this file)
```

## Using Generated Code

### In CMakeLists.txt

```cmake
# Set your MCU
set(ALLOY_MCU "STM32F103C8")

# Generated code is in src/generated/
set(ALLOY_GENERATED_DIR "${CMAKE_SOURCE_DIR}/src/generated/st/stm32f1/stm32f103c8")

# Add startup code
target_sources(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}/startup.cpp
)

# Include peripheral definitions
target_include_directories(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}
)
```

### In C++ Code

```cpp
#include "peripherals.hpp"

using namespace alloy::generated::stm32f103c8;

// Use GPIO
auto* gpioc = gpio::GPIOC;
gpioc->BSRR = (1U << 13);  // Set PC13

// Use USART
auto* uart = usart::USART1;
uart->DR = 'A';  // Send character
```

## Regenerating Code

To regenerate all MCU code:

```bash
cd tools/codegen

# Sync SVD files
python3 sync_svd.py --update

# Parse all SVDs
python3 parse_all_svds.py

# Generate all code
python3 generate_all.py --all
```

## Last Generated

{Path(__file__).stem}: Run `python3 generate_all.py --all` to update
"""

    index_path = output_dir / "INDEX.md"
    index_path.write_text(index_content)
    print_success(f"Generated {index_path.relative_to(REPO_ROOT)}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate code for all MCUs",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument('--vendor', '-v',
                        help='Generate only for specific vendor (e.g., STMicro)')
    parser.add_argument('--all', '-a', action='store_true',
                        help='Generate for all vendors')
    parser.add_argument('--clean', action='store_true',
                        help='Clean output directory first')

    args = parser.parse_args()

    if not args.vendor and not args.all:
        parser.print_help()
        sys.exit(1)

    print_header("Alloy MCU Code Generation - Batch Mode")

    # Clean if requested
    if args.clean and OUTPUT_DIR.exists():
        print_info(f"Cleaning {OUTPUT_DIR}...")
        import shutil
        shutil.rmtree(OUTPUT_DIR)

    # Create output directory
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Find all databases
    if not DATABASE_DIR.exists():
        print_error(f"Database directory not found: {DATABASE_DIR}")
        sys.exit(1)

    databases = list(DATABASE_DIR.glob("*.json"))

    if not databases:
        print_error(f"No database files found in {DATABASE_DIR}")
        sys.exit(1)

    print_info(f"Found {len(databases)} database(s)")

    # Process each database
    for db_path in databases:
        # Load to get vendor
        with open(db_path) as f:
            db = json.load(f)

        db_vendor = db.get('vendor', 'unknown')

        # Filter by vendor if specified
        if args.vendor and db_vendor != args.vendor:
            continue

        generate_for_database(db_path, db_vendor)

    # Generate READMEs
    print_header("Generating Documentation")

    for vendor_dir in OUTPUT_DIR.iterdir():
        if vendor_dir.is_dir() and not vendor_dir.name.startswith('.'):
            generate_vendor_readme(vendor_dir, vendor_dir.name)

    # Generate master index
    generate_index(OUTPUT_DIR)

    print_header("Generation Complete!")

    # Count total
    total_mcus = sum(1 for _ in OUTPUT_DIR.rglob("startup.cpp"))
    print_success(f"Generated code for {total_mcus} MCUs")
    print_info(f"Output: {OUTPUT_DIR.relative_to(REPO_ROOT)}")
    print_info("Commit these files to the repository!")


if __name__ == "__main__":
    main()
