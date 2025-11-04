#!/usr/bin/env python3
"""
Generate MCU Implementation Status Report

This script scans all available SVD files and compares them against
implemented GPIO support to generate a comprehensive status report.
"""

from pathlib import Path
from typing import Dict, Set, List
from cli.parsers.svd_discovery import discover_all_svds
import sys

REPO_ROOT = Path(__file__).parent.parent.parent.parent
HAL_DIR = REPO_ROOT / "src" / "hal" / "vendors"
OUTPUT_FILE = REPO_ROOT / "MCU_STATUS.md"


def get_implemented_mcus() -> Dict[str, Set[str]]:
    """
    Scan the hal/vendors directory to find all implemented MCU variants.
    Returns: Dict[vendor_name, Set[mcu_variant_names]]
    """
    implemented = {}

    if not HAL_DIR.exists():
        return implemented

    # Scan each vendor directory
    for vendor_dir in HAL_DIR.iterdir():
        if not vendor_dir.is_dir() or vendor_dir.name.startswith('.'):
            continue

        vendor_name = vendor_dir.name.upper()
        if vendor_name not in implemented:
            implemented[vendor_name] = set()

        # Scan each family directory (e.g., stm32f103, stm32f407)
        for family_dir in vendor_dir.iterdir():
            if not family_dir.is_dir() or family_dir.name.startswith('.'):
                continue

            # Scan each variant directory (e.g., stm32f103c8, stm32f407vg)
            for variant_dir in family_dir.iterdir():
                if not variant_dir.is_dir() or variant_dir.name.startswith('.'):
                    continue

                # Check if gpio.hpp exists (indicates implementation)
                if (variant_dir / "gpio.hpp").exists():
                    variant_name = variant_dir.name.upper()
                    implemented[vendor_name].add(variant_name)

    return implemented


def organize_svds_by_family(svds: Dict) -> Dict[str, Dict[str, List]]:
    """
    Organize SVD files by vendor and family.
    Returns: Dict[vendor, Dict[family, List[svd_info]]]
    """
    organized = {}

    for svd_name, svd_info in sorted(svds.items()):
        # Extract vendor and family from SVD name
        # Examples: STM32F103xx -> ST, STM32F1
        #           STM32F407 -> ST, STM32F4

        # Assume ST vendor for now (can be extended)
        vendor = "ST"

        # Extract family pattern
        # STM32F103xx -> STM32F1
        # STM32F407 -> STM32F4
        # STM32F7x2 -> STM32F7
        family = None

        if svd_name.startswith("STM32F0"):
            family = "STM32F0"
        elif svd_name.startswith("STM32F1"):
            family = "STM32F1"
        elif svd_name.startswith("STM32F2"):
            family = "STM32F2"
        elif svd_name.startswith("STM32F3"):
            family = "STM32F3"
        elif svd_name.startswith("STM32F4"):
            family = "STM32F4"
        elif svd_name.startswith("STM32F7"):
            family = "STM32F7"
        elif svd_name.startswith("STM32G0"):
            family = "STM32G0"
        elif svd_name.startswith("STM32G4"):
            family = "STM32G4"
        elif svd_name.startswith("STM32H7"):
            family = "STM32H7"
        elif svd_name.startswith("STM32L0"):
            family = "STM32L0"
        elif svd_name.startswith("STM32L1"):
            family = "STM32L1"
        elif svd_name.startswith("STM32L4"):
            family = "STM32L4"
        elif svd_name.startswith("STM32L5"):
            family = "STM32L5"
        elif svd_name.startswith("STM32U5"):
            family = "STM32U5"
        elif svd_name.startswith("STM32WB"):
            family = "STM32WB"
        elif svd_name.startswith("STM32WL"):
            family = "STM32WL"
        else:
            family = "Other"

        if vendor not in organized:
            organized[vendor] = {}

        if family not in organized[vendor]:
            organized[vendor][family] = []

        organized[vendor][family].append({
            'name': svd_name,
            'path': svd_info.file_path,
        })

    return organized


def get_common_variants_for_family(family: str) -> List[str]:
    """
    Return common MCU variant patterns for a given family.
    This helps identify which specific MCU variants we should support.
    """

    # Common package sizes and flash variants
    variants = {
        "STM32F0": [
            "STM32F030C6", "STM32F030C8", "STM32F030F4", "STM32F030K6",
            "STM32F031C4", "STM32F031C6", "STM32F031E6", "STM32F031F4",
            "STM32F051C4", "STM32F051C6", "STM32F051C8", "STM32F051K4",
        ],
        "STM32F1": [
            "STM32F103C4", "STM32F103C6", "STM32F103C8", "STM32F103CB",
            "STM32F103R4", "STM32F103R6", "STM32F103R8", "STM32F103RB",
            "STM32F103RC", "STM32F103RD", "STM32F103RE",
            "STM32F103T4", "STM32F103T6", "STM32F103T8", "STM32F103TB",
            "STM32F103V8", "STM32F103VB", "STM32F103VC", "STM32F103VD",
            "STM32F103VE", "STM32F103ZC", "STM32F103ZD", "STM32F103ZE",
        ],
        "STM32F2": [
            "STM32F205RB", "STM32F205RC", "STM32F205RE", "STM32F205RF",
            "STM32F205VB", "STM32F205VC", "STM32F205VE", "STM32F205VF",
            "STM32F205ZC", "STM32F205ZE", "STM32F205ZF",
            "STM32F207VC", "STM32F207VE", "STM32F207VF", "STM32F207ZC",
            "STM32F207ZE", "STM32F207ZF",
        ],
        "STM32F3": [
            "STM32F301C6", "STM32F301C8", "STM32F301K6", "STM32F301K8",
            "STM32F302C6", "STM32F302C8", "STM32F302CB", "STM32F302CC",
            "STM32F303C6", "STM32F303C8", "STM32F303CB", "STM32F303CC",
            "STM32F303RB", "STM32F303RC", "STM32F303RD", "STM32F303RE",
            "STM32F303VB", "STM32F303VC", "STM32F303VE",
        ],
        "STM32F4": [
            "STM32F401CB", "STM32F401CC", "STM32F401CD", "STM32F401CE",
            "STM32F401RB", "STM32F401RC", "STM32F401RD", "STM32F401RE",
            "STM32F405RG", "STM32F405VG", "STM32F405ZG",
            "STM32F407VE", "STM32F407VG", "STM32F407ZE", "STM32F407ZG",
            "STM32F411CC", "STM32F411CE", "STM32F411RC", "STM32F411RE",
            "STM32F412CE", "STM32F412CG", "STM32F412RE", "STM32F412RG",
            "STM32F413CG", "STM32F413CH", "STM32F413RG", "STM32F413RH",
            "STM32F415RG", "STM32F415VG", "STM32F415ZG",
            "STM32F417VE", "STM32F417VG", "STM32F417ZE", "STM32F417ZG",
            "STM32F427VG", "STM32F427VI", "STM32F427ZG", "STM32F427ZI",
            "STM32F429VE", "STM32F429VG", "STM32F429VI", "STM32F429ZE",
            "STM32F429ZG", "STM32F429ZI",
            "STM32F446MC", "STM32F446ME", "STM32F446RC", "STM32F446RE",
            "STM32F446VC", "STM32F446VE", "STM32F446ZC", "STM32F446ZE",
        ],
        "STM32F7": [
            "STM32F722IC", "STM32F722IE", "STM32F722RC", "STM32F722RE",
            "STM32F722VC", "STM32F722VE", "STM32F722ZC", "STM32F722ZE",
            "STM32F730I8", "STM32F730R8", "STM32F730V8", "STM32F730Z8",
            "STM32F732IE", "STM32F732RE", "STM32F732VE", "STM32F732ZE",
            "STM32F745VE", "STM32F745VG", "STM32F745ZE", "STM32F745ZG",
            "STM32F746BE", "STM32F746BG", "STM32F746IE", "STM32F746IG",
            "STM32F746NE", "STM32F746NG", "STM32F746VE", "STM32F746VG",
            "STM32F746ZE", "STM32F746ZG",
            "STM32F750N8", "STM32F750V8", "STM32F750Z8",
            "STM32F756BG", "STM32F756IG", "STM32F756NG", "STM32F756VG",
            "STM32F756ZG",
            "STM32F765BG", "STM32F765BI", "STM32F765IG", "STM32F765II",
            "STM32F765NG", "STM32F765NI", "STM32F765VG", "STM32F765VI",
            "STM32F765ZG", "STM32F765ZI",
            "STM32F767BG", "STM32F767BI", "STM32F767IG", "STM32F767II",
            "STM32F767NG", "STM32F767NI", "STM32F767VG", "STM32F767VI",
            "STM32F767ZG", "STM32F767ZI",
            "STM32F769AG", "STM32F769AI", "STM32F769BG", "STM32F769BI",
            "STM32F769IG", "STM32F769II", "STM32F769NG", "STM32F769NI",
        ],
        "STM32G0": [
            "STM32G030C6", "STM32G030C8", "STM32G030F6", "STM32G030J6",
            "STM32G030K6", "STM32G030K8",
            "STM32G031C4", "STM32G031C6", "STM32G031C8", "STM32G031F4",
            "STM32G031F6", "STM32G031F8", "STM32G031G4", "STM32G031G6",
            "STM32G031G8", "STM32G031J4", "STM32G031J6", "STM32G031K4",
            "STM32G031K6", "STM32G031K8", "STM32G031Y8",
        ],
        "STM32G4": [
            "STM32G431C6", "STM32G431C8", "STM32G431CB", "STM32G431K6",
            "STM32G431K8", "STM32G431KB", "STM32G431R6", "STM32G431R8",
            "STM32G431RB", "STM32G431V6", "STM32G431V8", "STM32G431VB",
            "STM32G441CB", "STM32G441KB", "STM32G441RB", "STM32G441VB",
            "STM32G473CB", "STM32G473CC", "STM32G473CE", "STM32G473MB",
            "STM32G473MC", "STM32G473ME", "STM32G473PB", "STM32G473PC",
            "STM32G473PE", "STM32G473QB", "STM32G473QC", "STM32G473QE",
            "STM32G473RB", "STM32G473RC", "STM32G473RE", "STM32G473VB",
            "STM32G473VC", "STM32G473VE",
        ],
        "STM32H7": [
            "STM32H743AG", "STM32H743AI", "STM32H743BG", "STM32H743BI",
            "STM32H743IG", "STM32H743II", "STM32H743VG", "STM32H743VI",
            "STM32H743XG", "STM32H743XI", "STM32H743ZG", "STM32H743ZI",
            "STM32H753AI", "STM32H753BI", "STM32H753II", "STM32H753VI",
            "STM32H753XI", "STM32H753ZI",
        ],
        "STM32L0": [
            "STM32L011D3", "STM32L011D4", "STM32L011E3", "STM32L011E4",
            "STM32L011F3", "STM32L011F4", "STM32L011G3", "STM32L011G4",
            "STM32L011K3", "STM32L011K4",
            "STM32L031C4", "STM32L031C6", "STM32L031E4", "STM32L031E6",
            "STM32L031F4", "STM32L031F6", "STM32L031G4", "STM32L031G6",
            "STM32L031K4", "STM32L031K6",
        ],
        "STM32L1": [
            "STM32L151C6", "STM32L151C8", "STM32L151CB", "STM32L151CC",
            "STM32L151R6", "STM32L151R8", "STM32L151RB", "STM32L151RC",
            "STM32L151V6", "STM32L151V8", "STM32L151VB", "STM32L151VC",
            "STM32L152C6", "STM32L152C8", "STM32L152CB", "STM32L152CC",
            "STM32L152R6", "STM32L152R8", "STM32L152RB", "STM32L152RC",
            "STM32L152V6", "STM32L152V8", "STM32L152VB", "STM32L152VC",
        ],
        "STM32L4": [
            "STM32L412C8", "STM32L412CB", "STM32L412K8", "STM32L412KB",
            "STM32L412R8", "STM32L412RB", "STM32L412T8", "STM32L412TB",
            "STM32L422CB", "STM32L422KB", "STM32L422RB", "STM32L422TB",
            "STM32L431CB", "STM32L431CC", "STM32L431KB", "STM32L431KC",
            "STM32L431RB", "STM32L431RC", "STM32L431VC",
            "STM32L432KB", "STM32L432KC", "STM32L433CB", "STM32L433CC",
            "STM32L433RB", "STM32L433RC", "STM32L433VC",
            "STM32L451CC", "STM32L451CE", "STM32L451RC", "STM32L451RE",
            "STM32L451VC", "STM32L451VE",
            "STM32L452CC", "STM32L452CE", "STM32L452RC", "STM32L452RE",
            "STM32L452VC", "STM32L452VE",
            "STM32L471QE", "STM32L471QG", "STM32L471RE", "STM32L471RG",
            "STM32L471VE", "STM32L471VG", "STM32L471ZE", "STM32L471ZG",
            "STM32L475RC", "STM32L475RE", "STM32L475RG", "STM32L475VC",
            "STM32L475VE", "STM32L475VG",
            "STM32L476JE", "STM32L476JG", "STM32L476ME", "STM32L476MG",
            "STM32L476QE", "STM32L476QG", "STM32L476RC", "STM32L476RE",
            "STM32L476RG", "STM32L476VC", "STM32L476VE", "STM32L476VG",
            "STM32L476ZE", "STM32L476ZG",
        ],
        "STM32L5": [
            "STM32L552CC", "STM32L552CE", "STM32L552ME", "STM32L552QC",
            "STM32L552QE", "STM32L552RC", "STM32L552RE", "STM32L552VC",
            "STM32L552VE", "STM32L552ZC", "STM32L552ZE",
            "STM32L562CE", "STM32L562ME", "STM32L562QE", "STM32L562RE",
            "STM32L562VE", "STM32L562ZE",
        ],
        "STM32U5": [
            "STM32U575AG", "STM32U575AI", "STM32U575CG", "STM32U575CI",
            "STM32U575OG", "STM32U575OI", "STM32U575QG", "STM32U575QI",
            "STM32U575RG", "STM32U575RI", "STM32U575VG", "STM32U575VI",
            "STM32U575ZG", "STM32U575ZI",
            "STM32U585AI", "STM32U585CI", "STM32U585OI", "STM32U585QI",
            "STM32U585RI", "STM32U585VI", "STM32U585ZI",
        ],
        "STM32WB": [
            "STM32WB10CC", "STM32WB15CC", "STM32WB30CE", "STM32WB35CC",
            "STM32WB35CE", "STM32WB50CG", "STM32WB55CC", "STM32WB55CE",
            "STM32WB55CG", "STM32WB55RC", "STM32WB55RE", "STM32WB55RG",
            "STM32WB55VC", "STM32WB55VE", "STM32WB55VG", "STM32WB55VY",
        ],
        "STM32WL": [
            "STM32WL54JC", "STM32WL54CC", "STM32WL55CC", "STM32WL55JC",
            "STM32WLE4C8", "STM32WLE4CB", "STM32WLE4CC", "STM32WLE5C8",
            "STM32WLE5CB", "STM32WLE5CC", "STM32WLE5J8", "STM32WLE5JB",
            "STM32WLE5JC",
        ],
    }

    return variants.get(family, [])


def generate_status_report():
    """Generate comprehensive MCU implementation status report."""

    print("=" * 80)
    print("🔍 MCU Implementation Status Report Generator")
    print("=" * 80)
    print()

    # Discover all available SVDs
    print("📦 Discovering available SVD files...")
    all_svds = discover_all_svds()
    print(f"   Found {len(all_svds)} SVD files\n")

    # Get implemented MCUs
    print("✅ Scanning implemented MCU variants...")
    implemented = get_implemented_mcus()
    total_implemented = sum(len(variants) for variants in implemented.values())
    print(f"   Found {total_implemented} implemented variants\n")

    # Organize SVDs by vendor and family
    print("📊 Organizing by vendor and family...")
    organized = organize_svds_by_family(all_svds)
    print()

    # Generate markdown report
    print(f"📝 Generating report: {OUTPUT_FILE}")

    with open(OUTPUT_FILE, 'w') as f:
        f.write("# MCU GPIO Implementation Status\n\n")
        f.write("This document tracks the implementation status of GPIO support ")
        f.write("for all available microcontrollers.\n\n")
        f.write(f"**Total SVD Files Available:** {len(all_svds)}  \n")
        f.write(f"**Total MCU Variants Implemented:** {total_implemented}\n\n")
        f.write("---\n\n")

        # Process each vendor
        for vendor in sorted(organized.keys()):
            f.write(f"## {vendor} (STMicroelectronics)\n\n")

            vendor_impl = implemented.get(vendor, set())

            # Process each family
            for family in sorted(organized[vendor].keys()):
                svd_list = organized[vendor][family]

                # Count implemented variants for this family
                family_implemented = [v for v in vendor_impl if v.startswith(family.replace('STM32', 'STM32'))]
                impl_count = len(family_implemented)

                # Get common variants
                common_variants = get_common_variants_for_family(family)
                total_variants = len(common_variants)

                # Calculate percentage
                percentage = (impl_count / total_variants * 100) if total_variants > 0 else 0

                # Family header with progress
                f.write(f"### {family}\n\n")
                f.write(f"**SVD Files:** {len(svd_list)}  \n")
                f.write(f"**Implemented:** {impl_count}/{total_variants} ({percentage:.1f}%)  \n")
                f.write(f"**Progress:** ")

                # Progress bar
                bar_length = 20
                filled = int(bar_length * impl_count / total_variants) if total_variants > 0 else 0
                bar = "█" * filled + "░" * (bar_length - filled)
                f.write(f"`{bar}`\n\n")

                # SVD files for this family
                f.write(f"<details>\n")
                f.write(f"<summary>SVD Files ({len(svd_list)})</summary>\n\n")
                for svd in sorted(svd_list, key=lambda x: x['name']):
                    f.write(f"- `{svd['name']}` - `{svd['path'].name}`\n")
                f.write(f"\n</details>\n\n")

                # Common MCU variants status
                if common_variants:
                    f.write(f"<details>\n")
                    f.write(f"<summary>Common MCU Variants ({len(common_variants)})</summary>\n\n")
                    f.write(f"| MCU Variant | Status |\n")
                    f.write(f"|-------------|--------|\n")

                    for variant in sorted(common_variants):
                        is_impl = variant.upper() in vendor_impl
                        status = "✅ Implemented" if is_impl else "⬜ Not Implemented"
                        f.write(f"| {variant} | {status} |\n")

                    f.write(f"\n</details>\n\n")

                # Implemented variants (if any)
                if family_implemented:
                    f.write(f"<details>\n")
                    f.write(f"<summary>✅ Implemented Variants ({len(family_implemented)})</summary>\n\n")
                    for variant in sorted(family_implemented):
                        f.write(f"- ✅ {variant}\n")
                    f.write(f"\n</details>\n\n")

                f.write("---\n\n")

        # Summary section
        f.write("## Summary\n\n")
        f.write("### Implementation Status by Family\n\n")
        f.write("| Family | Implemented | Total Variants | Progress |\n")
        f.write("|--------|-------------|----------------|----------|\n")

        for vendor in sorted(organized.keys()):
            vendor_impl = implemented.get(vendor, set())
            for family in sorted(organized[vendor].keys()):
                common_variants = get_common_variants_for_family(family)
                total_variants = len(common_variants)
                family_implemented = [v for v in vendor_impl if v.startswith(family.replace('STM32', 'STM32'))]
                impl_count = len(family_implemented)
                percentage = (impl_count / total_variants * 100) if total_variants > 0 else 0

                bar_length = 10
                filled = int(bar_length * impl_count / total_variants) if total_variants > 0 else 0
                bar = "█" * filled + "░" * (bar_length - filled)

                f.write(f"| {family} | {impl_count} | {total_variants} | `{bar}` {percentage:.0f}% |\n")

        f.write("\n---\n\n")
        f.write("*Last updated: Auto-generated by `tools/codegen/generate_mcu_status.py`*\n")

    print(f"✅ Report generated successfully!")
    print(f"\n📄 Output: {OUTPUT_FILE}")
    print()

    # Print summary to console
    print("=" * 80)
    print("📊 Summary")
    print("=" * 80)
    for vendor in sorted(organized.keys()):
        print(f"\n{vendor}:")
        vendor_impl = implemented.get(vendor, set())
        for family in sorted(organized[vendor].keys()):
            common_variants = get_common_variants_for_family(family)
            total_variants = len(common_variants)
            family_implemented = [v for v in vendor_impl if v.startswith(family.replace('STM32', 'STM32'))]
            impl_count = len(family_implemented)
            percentage = (impl_count / total_variants * 100) if total_variants > 0 else 0
            print(f"  {family:12} - {impl_count:3}/{total_variants:3} ({percentage:5.1f}%)")

    print()
    return 0


if __name__ == "__main__":
    sys.exit(generate_status_report())
