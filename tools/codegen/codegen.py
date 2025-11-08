#!/usr/bin/env python3
"""
Alloy Code Generator - Unified CLI

Single entry point for all code generation commands.

Usage:
    python3 codegen.py generate [OPTIONS]
    python3 codegen.py generate-complete [OPTIONS]  # RECOMMENDED
    python3 codegen.py status
    python3 codegen.py clean [OPTIONS]
    python3 codegen.py vendors
    python3 codegen.py test-parser <svd_file>

Examples:
    # Generate everything + format + validate (RECOMMENDED)
    python3 codegen.py generate-complete

    # Generate everything (no format/validate)
    python3 codegen.py generate

    # Generate only startup
    python3 codegen.py generate --startup

    # Generate only pins for ST
    python3 codegen.py generate --pins --vendor st

    # See status
    python3 codegen.py status

    # Clean generated files
    python3 codegen.py clean --dry-run
"""

import sys
import argparse
from pathlib import Path

# Add codegen to path
CODEGEN_DIR = Path(__file__).parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info
from cli.core.version import VERSION


# ============================================================================
# COMMAND: generate
# ============================================================================

def cmd_generate(args):
    """Generate code for vendors"""
    from cli.core.progress import ProgressTracker, set_global_tracker

    # Quiet mode disables verbose
    if args.quiet:
        args.verbose = False

    # Initialize progress tracker
    repo_root = CODEGEN_DIR.parent.parent
    tracker = ProgressTracker(
        verbose=args.verbose and not args.quiet,
        repo_root=repo_root,
        enable_manifest=not args.no_manifest
    )
    set_global_tracker(tracker)

    success = True

    try:
        # IMPORTANT: Generate pins FIRST because other generators depend on pin_functions.hpp
        # Generate pins if requested
        if args.all or args.pins:
            # ST
            if not args.vendor or args.vendor == "st":
                print_header("Generating STM32 Pin Headers")
                try:
                    import cli.vendors.st.generate_all_st_pins as st_gen
                    st_gen.main()
                except Exception as e:
                    print_error(f"ST generation failed: {e}")
                    if args.verbose:
                        import traceback
                        traceback.print_exc()
                    success = False

            # Atmel
            if not args.vendor or args.vendor in ["atmel", "microchip"]:
                print_header("Generating Atmel/Microchip Pin Headers")
                try:
                    from cli.vendors.atmel.generate_samd21_pins import main as gen_samd21
                    from cli.vendors.atmel.generate_same70_pins import main as gen_same70
                    gen_samd21()
                    gen_same70()
                except Exception as e:
                    print_error(f"Atmel generation failed: {e}")
                    if args.verbose:
                        import traceback
                        traceback.print_exc()
                    success = False

        # Generate pin alternate functions if requested
        if args.all or args.pin_functions:
            print_header("Generating Pin Alternate Function Mappings")
            from cli.generators.generate_pin_functions import generate_for_board_mcus as gen_pin_funcs
            result = gen_pin_funcs(args.verbose, tracker)
            if result != 0:
                success = False

        # Generate registers if requested (needs pin_functions.hpp)
        if args.all or args.registers:
            print_header("Generating Register Structures")
            from cli.generators.generate_registers import generate_for_board_mcus as gen_regs
            result = gen_regs(args.verbose, tracker)
            if result != 0:
                success = False

        # Generate startup if requested (needs pin_functions.hpp)
        if args.all or args.startup:
            print_header("Generating Startup Code")
            from cli.generators.generate_startup import generate_for_board_mcus
            result = generate_for_board_mcus(args.verbose, tracker)
            if result != 0:
                success = False

        # Generate enumerations if requested (needs pin_functions.hpp)
        if args.all or args.enums:
            print_header("Generating Enumeration Definitions")
            from cli.generators.generate_enums import generate_for_board_mcus as gen_enums
            result = gen_enums(args.verbose, tracker)
            if result != 0:
                success = False

        # Generate complete register map if requested
        if args.all or args.register_map:
            print_header("Generating Complete Register Maps")
            from cli.generators.generate_register_map import generate_for_board_mcus as gen_map
            result = gen_map(args.verbose, tracker)
            if result != 0:
                success = False

        # Show summary
        tracker.complete_generation()
        tracker.print_summary()

        if success:
            print_success("\n✅ Code generation complete!")
        else:
            print_error("\n❌ Some generators failed (see above)")

        # Show manifest info
        if tracker.manifest_manager and not args.no_manifest:
            manifest_path = tracker.manifest_manager.manifest_path
            print_info(f"\n📝 Manifest: {manifest_path}")
            print_info(f"   Run 'python3 codegen.py clean --stats' to see details")

        return 0 if success else 1

    except KeyboardInterrupt:
        print_error("\n\n❌ Generation interrupted")
        return 1
    except Exception as e:
        print_error(f"\n\n❌ Unexpected error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1


# ============================================================================
# COMMAND: status
# ============================================================================

def cmd_status(args):
    """Show code generation status"""
    from cli.commands.status import execute
    return execute(args)


# ============================================================================
# COMMAND: clean
# ============================================================================

def cmd_clean(args):
    """Clean generated files"""
    from cli.commands.clean import execute
    return execute(args)


# ============================================================================
# COMMAND: vendors
# ============================================================================

def cmd_vendors(args):
    """Show vendor information"""
    from cli.commands.vendors import execute
    return execute(args)


# ============================================================================
# COMMAND: test-parser
# ============================================================================

def cmd_test_parser(args):
    """Test SVD parser on a file"""
    from cli.parsers.generic_svd import parse_svd
    from cli.core.config import SVD_DIR

    svd_path = Path(args.svd_file)
    if not svd_path.is_absolute():
        svd_path = SVD_DIR / svd_path

    if not svd_path.exists():
        print_error(f"SVD file not found: {svd_path}")
        return 1

    print_header(f"Testing Parser: {svd_path.name}")

    device = parse_svd(svd_path)
    if not device:
        print_error("Failed to parse SVD file")
        return 1

    print_success("Successfully parsed!")
    print()
    print(f"  Device:        {device.name}")
    print(f"  Vendor:        {device.vendor} → {device.vendor_normalized}")
    print(f"  Family:        {device.family}")
    print(f"  Board MCU:     {'Yes' if device.is_board_mcu else 'No'}")
    if device.description:
        print(f"  Description:   {device.description}")
    print()
    print(f"  CPU:           {device.cpu_name or 'N/A'}")
    print(f"  FPU:           {'Yes' if device.cpu_fpuPresent else 'No'}")
    print(f"  MPU:           {'Yes' if device.cpu_mpuPresent else 'No'}")
    print()
    print(f"  Peripherals:   {len(device.peripherals)}")
    print(f"  Interrupts:    {len(device.interrupts)}")
    print(f"  Memory Rgns:   {len(device.memory_regions)}")

    if args.verbose:
        print()
        print("Interrupts (first 10):")
        for irq in device.interrupts[:10]:
            print(f"    {irq.value:3d}: {irq.name}")
        if len(device.interrupts) > 10:
            print(f"    ... and {len(device.interrupts) - 10} more")

        print()
        print("Peripherals (first 10):")
        for name, periph in list(device.peripherals.items())[:10]:
            print(f"    {name:20s} @ 0x{periph.base_address:08X}")
        if len(device.peripherals) > 10:
            print(f"    ... and {len(device.peripherals) - 10} more")

    return 0


# ============================================================================
# COMMAND: generate-complete
# ============================================================================

def cmd_generate_complete(args):
    """
    Generate everything in the correct sequence, then format and validate.

    This is the recommended way to regenerate all code from scratch.

    Sequence:
      1. Generate vendor code (pins, startup, registers, enums)
      2. Generate platform HAL implementations
      3. Format all generated code (clang-format)
      4. Validate with clang-tidy (startup files)
    """
    import subprocess
    import time

    print_header("Complete Code Generation Pipeline")
    print()
    print("This will:")
    print("  1. Generate all vendor code (pins, startup, registers, enums)")
    print("  2. Generate all platform HAL implementations")
    print("  3. Format all generated code with clang-format")
    print("  4. Validate startup files with clang-tidy")
    print()

    start_time = time.time()
    repo_root = CODEGEN_DIR.parent.parent

    # ========================================================================
    # STEP 1: Generate vendor code
    # ========================================================================
    print_header("Step 1/4: Generating Vendor Code")
    print()

    result = subprocess.run(
        [sys.executable, str(CODEGEN_DIR / "codegen.py"), "generate", "--all"],
        cwd=CODEGEN_DIR
    )

    if result.returncode != 0:
        print_error("❌ Vendor code generation failed!")
        return 1

    print_success("✅ Vendor code generation complete")
    print()

    # ========================================================================
    # STEP 2: Generate platform HAL
    # ========================================================================
    print_header("Step 2/4: Generating Platform HAL")
    print()

    unified_gen = CODEGEN_DIR / "cli" / "generators" / "unified_generator.py"
    if unified_gen.exists():
        result = subprocess.run(
            [sys.executable, str(unified_gen)],
            cwd=CODEGEN_DIR
        )

        if result.returncode != 0:
            print_error("❌ Platform HAL generation failed!")
            if not args.continue_on_error:
                return 1
        else:
            print_success("✅ Platform HAL generation complete")
    else:
        print_info("⚠️  Unified generator not found, skipping platform HAL")

    # Generate universal HAL wrappers
    print()
    print_info("Generating universal HAL wrappers...")
    hal_wrapper_gen = CODEGEN_DIR / "cli" / "generators" / "generate_hal_wrappers.py"
    if hal_wrapper_gen.exists():
        result = subprocess.run(
            [sys.executable, str(hal_wrapper_gen)],
            cwd=CODEGEN_DIR
        )

        if result.returncode != 0:
            print_error("❌ HAL wrapper generation failed!")
            if not args.continue_on_error:
                return 1
        else:
            print_success("✅ HAL wrapper generation complete")
    else:
        print_info("⚠️  HAL wrapper generator not found, skipping")

    print()

    # ========================================================================
    # STEP 3: Format generated code
    # ========================================================================
    if not args.skip_format:
        print_header("Step 3/4: Formatting Generated Code")
        print()

        format_script = CODEGEN_DIR / "scripts" / "format_generated_code.sh"
        if format_script.exists():
            result = subprocess.run(
                ["bash", str(format_script)],
                cwd=repo_root
            )

            if result.returncode != 0:
                print_error("❌ Code formatting failed!")
                if not args.continue_on_error:
                    return 1
            else:
                print_success("✅ Code formatting complete")
        else:
            print_info("⚠️  Format script not found, skipping formatting")

        print()
    else:
        print_info("⏭️  Skipping code formatting (--skip-format)")
        print()

    # ========================================================================
    # STEP 4: Validate with clang-tidy
    # ========================================================================
    if not args.skip_validate:
        print_header("Step 4/4: Validating with clang-tidy")
        print()

        validate_script = CODEGEN_DIR / "scripts" / "validate_clang_tidy.sh"
        if validate_script.exists():
            result = subprocess.run(
                ["bash", str(validate_script)],
                cwd=repo_root
            )

            if result.returncode != 0:
                print_error("❌ Validation failed!")
                print_info("   Some files may have clang-tidy warnings")
                print_info("   Run manually: bash tools/codegen/scripts/validate_clang_tidy.sh")
                if not args.continue_on_error:
                    return 1
            else:
                print_success("✅ Validation complete")
        else:
            print_info("⚠️  Validation script not found, skipping validation")

        print()
    else:
        print_info("⏭️  Skipping validation (--skip-validate)")
        print()

    # ========================================================================
    # SUMMARY
    # ========================================================================
    elapsed = time.time() - start_time
    print_header("Generation Complete!")
    print()
    print(f"  ⏱️  Total time: {elapsed:.1f}s")
    print()
    print_success("✅ All steps completed successfully!")
    print()
    print("Next steps:")
    print("  1. Review generated files: src/hal/vendors/")
    print("  2. Review platform HAL: src/hal/platform/")
    print("  3. Build and test: cmake --build build")
    print()

    return 0


# ============================================================================
# COMMAND: config
# ============================================================================

def cmd_config(args):
    """Show configuration"""
    from cli.core.config import (
        BOARD_MCUS,
        VENDOR_NAME_MAP,
        FAMILY_PATTERNS,
        normalize_vendor,
        detect_family
    )

    print_header("Alloy Code Generator Configuration")

    print("\n📋 Board MCUs:")
    for mcu in BOARD_MCUS:
        print(f"  - {mcu}")

    print(f"\n🏢 Vendors Supported: {len(VENDOR_NAME_MAP)} name variations")
    if args.verbose:
        print("\nVendor mappings:")
        for original, normalized in sorted(set(VENDOR_NAME_MAP.items())):
            print(f"  {original:40s} → {normalized}")

    print(f"\n🔍 Family Detection Patterns: {len(FAMILY_PATTERNS)}")

    if args.test:
        print("\n🧪 Testing detection:")
        test_cases = [
            "STM32F103C8",
            "ATSAMD21G18A",
            "nRF52840",
            "ESP32-C3",
            "RP2040",
            "ATSAME70Q21",
        ]
        for mcu in test_cases:
            vendor = normalize_vendor("ST" if "STM32" in mcu else "Atmel")
            family = detect_family(mcu)
            print(f"  {mcu:20s} → vendor: {vendor:10s} family: {family}")

    return 0


# ============================================================================
# MAIN CLI
# ============================================================================

def main():
    """Main CLI entry point"""

    parser = argparse.ArgumentParser(
        prog='codegen',
        description='Alloy Code Generator - Unified CLI',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate everything + format + validate (RECOMMENDED)
  python3 codegen.py generate-complete

  # Skip validation
  python3 codegen.py generate-complete --skip-validate

  # Generate everything (no format/validate)
  python3 codegen.py generate

  # Generate only startup
  python3 codegen.py generate --startup

  # Generate pins for ST only
  python3 codegen.py generate --pins --vendor st

  # Generate with verbose output
  python3 codegen.py generate --verbose

  # See status
  python3 codegen.py status

  # Clean generated files (dry-run)
  python3 codegen.py clean --dry-run

  # Test parser on SVD file
  python3 codegen.py test-parser STMicro/STM32F103.svd --verbose

  # Show configuration
  python3 codegen.py config --test

For more help on a command:
  python3 codegen.py <command> --help
        """
    )

    parser.add_argument('--version', action='version', version=f'Alloy Codegen {VERSION}')

    # Subcommands
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')

    # ========================================================================
    # COMMAND: generate
    # ========================================================================
    gen_parser = subparsers.add_parser(
        'generate',
        aliases=['gen', 'g'],
        help='Generate code for vendors',
        description='Generate startup code and/or pin headers for supported vendors'
    )

    gen_parser.add_argument(
        '--all',
        action='store_true',
        default=True,
        help='Generate everything (startup + registers + enums + pins) [default]'
    )

    gen_parser.add_argument(
        '--startup',
        action='store_true',
        help='Generate only startup code'
    )

    gen_parser.add_argument(
        '--registers',
        action='store_true',
        help='Generate only register structures and bitfields'
    )

    gen_parser.add_argument(
        '--enums',
        action='store_true',
        help='Generate only enumeration definitions'
    )

    gen_parser.add_argument(
        '--pin-functions',
        action='store_true',
        help='Generate only pin alternate function mappings'
    )

    gen_parser.add_argument(
        '--register-map',
        action='store_true',
        help='Generate only complete register map (single include)'
    )

    gen_parser.add_argument(
        '--pins',
        action='store_true',
        help='Generate only pin headers'
    )

    gen_parser.add_argument(
        '--vendor',
        choices=['st', 'atmel', 'microchip'],
        help='Generate for specific vendor only (with --pins)'
    )

    gen_parser.add_argument(
        '--no-manifest',
        action='store_true',
        help='Disable manifest tracking'
    )

    gen_parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )

    gen_parser.add_argument(
        '--quiet', '-q',
        action='store_true',
        help='Quiet mode - minimal output (faster)'
    )

    # ========================================================================
    # COMMAND: status
    # ========================================================================
    status_parser = subparsers.add_parser(
        'status',
        aliases=['st'],
        help='Show code generation status',
        description='Show status of code generation and available MCUs'
    )

    status_parser.add_argument(
        '--output',
        type=Path,
        default=Path('MCU_STATUS.md'),
        help='Output file path (default: MCU_STATUS.md)'
    )

    status_parser.add_argument(
        '--vendor',
        type=str,
        choices=['st', 'atmel', 'microchip', 'all'],
        default='all',
        help='Show status for specific vendor (default: all)'
    )

    status_parser.add_argument(
        '--format',
        type=str,
        choices=['markdown', 'json', 'text'],
        default='markdown',
        help='Output format (default: markdown)'
    )

    # ========================================================================
    # COMMAND: clean
    # ========================================================================
    clean_parser = subparsers.add_parser(
        'clean',
        help='Clean generated files',
        description='Clean generated files tracked in manifest'
    )

    clean_parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be deleted without actually deleting'
    )

    clean_parser.add_argument(
        '--generator',
        help='Clean files from specific generator only'
    )

    clean_parser.add_argument(
        '--stats',
        action='store_true',
        help='Show manifest statistics'
    )

    clean_parser.add_argument(
        '--validate',
        action='store_true',
        help='Validate file checksums'
    )

    # ========================================================================
    # COMMAND: vendors
    # ========================================================================
    vendors_parser = subparsers.add_parser(
        'vendors',
        help='Show vendor information',
        description='Show supported vendors and their MCUs'
    )

    # ========================================================================
    # COMMAND: test-parser
    # ========================================================================
    test_parser = subparsers.add_parser(
        'test-parser',
        help='Test SVD parser',
        description='Test the generic SVD parser on a specific file'
    )

    test_parser.add_argument(
        'svd_file',
        help='SVD file to parse (relative to SVD directory or absolute path)'
    )

    test_parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Show detailed output'
    )

    # ========================================================================
    # COMMAND: generate-complete
    # ========================================================================
    complete_parser = subparsers.add_parser(
        'generate-complete',
        aliases=['genall', 'full'],
        help='Generate everything + format + validate',
        description='Generate all code, format with clang-format, and validate with clang-tidy'
    )

    complete_parser.add_argument(
        '--skip-format',
        action='store_true',
        help='Skip clang-format step'
    )

    complete_parser.add_argument(
        '--skip-validate',
        action='store_true',
        help='Skip clang-tidy validation step'
    )

    complete_parser.add_argument(
        '--continue-on-error',
        action='store_true',
        help='Continue even if a step fails'
    )

    # ========================================================================
    # COMMAND: config
    # ========================================================================
    config_parser = subparsers.add_parser(
        'config',
        help='Show configuration',
        description='Show code generator configuration'
    )

    config_parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Show detailed configuration'
    )

    config_parser.add_argument(
        '--test',
        action='store_true',
        help='Test vendor and family detection'
    )

    # Parse arguments
    args = parser.parse_args()

    # If no command specified, show help
    if not args.command:
        parser.print_help()
        return 1

    # Execute command
    try:
        if args.command in ['generate', 'gen', 'g']:
            # Handle generate flags
            if args.startup:
                args.all = False
            if args.registers:
                args.all = False
            if args.enums:
                args.all = False
            if args.pin_functions:
                args.all = False
            if args.register_map:
                args.all = False
            if args.pins:
                args.all = False
            # If none specified, generate all
            if not args.startup and not args.registers and not args.enums and not args.pin_functions and not args.register_map and not args.pins:
                args.all = True
            return cmd_generate(args)
        elif args.command in ['generate-complete', 'genall', 'full']:
            return cmd_generate_complete(args)
        elif args.command in ['status', 'st']:
            return cmd_status(args)
        elif args.command == 'clean':
            return cmd_clean(args)
        elif args.command == 'vendors':
            return cmd_vendors(args)
        elif args.command == 'test-parser':
            return cmd_test_parser(args)
        elif args.command == 'config':
            return cmd_config(args)
        else:
            print_error(f"Unknown command: {args.command}")
            parser.print_help()
            return 1

    except Exception as e:
        print_error(f"Command failed: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
