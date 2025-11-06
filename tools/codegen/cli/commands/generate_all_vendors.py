#!/usr/bin/env python3
"""
Unified Vendor Generator Command

Generate code for all vendors with a single command.
Integrates all vendor-specific generators with progress tracking and manifest.
"""

import sys
import argparse
from pathlib import Path

# Add parent to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info
from cli.core.progress import ProgressTracker, set_global_tracker

# Import vendor generators
from cli.generators.generate_startup import generate_for_board_mcus
from cli.vendors.st.generate_all_st_pins import main as generate_st_pins
from cli.vendors.atmel.generate_samd21_pins import main as generate_samd21_pins
from cli.vendors.atmel.generate_same70_pins import main as generate_same70_pins


def generate_all(args):
    """Generate code for all vendors"""

    print_header("Alloy Unified Code Generator")
    print_info("Generating code for all supported vendors...\n")

    # Initialize progress tracker with manifest support
    repo_root = CODEGEN_DIR.parent.parent
    tracker = ProgressTracker(
        verbose=not args.quiet,
        repo_root=repo_root,
        enable_manifest=True
    )
    set_global_tracker(tracker)

    success = True

    try:
        # 1. Generate startup code for all board MCUs
        if not args.pins_only:
            print_header("Step 1: Startup Code")
            result = generate_for_board_mcus(args.verbose, tracker)
            if result != 0:
                success = False
                print_error("Startup generation failed")

        # 2. Generate ST pin headers
        if not args.startup_only:
            print_header("Step 2: STM32 Pin Headers")
            try:
                # ST generator uses print directly, so we call it
                import cli.vendors.st.generate_all_st_pins as st_gen
                st_gen.main()
            except Exception as e:
                print_error(f"ST pin generation failed: {e}")
                success = False

        # 3. Generate Atmel pin headers
        if not args.startup_only and not args.st_only:
            print_header("Step 3: Atmel/Microchip Pin Headers")

            # SAMD21
            try:
                generate_samd21_pins()
            except Exception as e:
                print_error(f"SAMD21 generation failed: {e}")
                success = False

            # SAME70
            try:
                generate_same70_pins()
            except Exception as e:
                print_error(f"SAME70 generation failed: {e}")
                success = False

        # Complete and show summary
        tracker.complete_generation()
        tracker.print_summary()

        if success:
            print_success("\n✅ All code generation complete!")
        else:
            print_error("\n❌ Some generators failed (see above)")

        # Save manifest
        if tracker.manifest_manager:
            manifest_path = tracker.manifest_manager.manifest_path
            print_info(f"\n📝 Manifest saved to: {manifest_path}")
            print_info(f"   Use 'python3 cli/commands/clean.py --stats' to see details")

        return 0 if success else 1

    except KeyboardInterrupt:
        print_error("\n\n❌ Generation interrupted by user")
        return 1
    except Exception as e:
        print_error(f"\n\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1


def generate_startup_only(args):
    """Generate only startup code"""

    print_header("Generating Startup Code")

    repo_root = CODEGEN_DIR.parent.parent
    tracker = ProgressTracker(
        verbose=not args.quiet,
        repo_root=repo_root,
        enable_manifest=True
    )
    set_global_tracker(tracker)

    result = generate_for_board_mcus(args.verbose, tracker)

    tracker.complete_generation()
    tracker.print_summary()

    return result


def generate_pins_only(args):
    """Generate only pin headers"""

    print_header("Generating Pin Headers")

    repo_root = CODEGEN_DIR.parent.parent
    tracker = ProgressTracker(
        verbose=not args.quiet,
        repo_root=repo_root,
        enable_manifest=True
    )
    set_global_tracker(tracker)

    success = True

    # ST
    if not args.vendor or args.vendor == "st":
        print_header("STM32 Pin Headers")
        try:
            import cli.vendors.st.generate_all_st_pins as st_gen
            st_gen.main()
        except Exception as e:
            print_error(f"ST generation failed: {e}")
            success = False

    # Atmel
    if not args.vendor or args.vendor in ["atmel", "microchip"]:
        print_header("Atmel/Microchip Pin Headers")
        try:
            generate_samd21_pins()
            generate_same70_pins()
        except Exception as e:
            print_error(f"Atmel generation failed: {e}")
            success = False

    tracker.complete_generation()
    tracker.print_summary()

    return 0 if success else 1


def main():
    """Main entry point"""

    parser = argparse.ArgumentParser(
        description="Unified code generator for all vendors",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate everything
  python3 cli/commands/generate_all_vendors.py

  # Generate only startup code
  python3 cli/commands/generate_all_vendors.py --startup-only

  # Generate only pin headers
  python3 cli/commands/generate_all_vendors.py --pins-only

  # Generate for specific vendor
  python3 cli/commands/generate_all_vendors.py --pins-only --vendor st

  # Quiet mode
  python3 cli/commands/generate_all_vendors.py --quiet

  # Verbose mode
  python3 cli/commands/generate_all_vendors.py --verbose
        """
    )

    parser.add_argument(
        '--startup-only',
        action='store_true',
        help='Generate only startup code (skip pin headers)'
    )

    parser.add_argument(
        '--pins-only',
        action='store_true',
        help='Generate only pin headers (skip startup)'
    )

    parser.add_argument(
        '--vendor',
        choices=['st', 'atmel', 'microchip'],
        help='Generate for specific vendor only (with --pins-only)'
    )

    parser.add_argument(
        '--st-only',
        action='store_true',
        help='Generate only for ST (startup + pins)'
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )

    parser.add_argument(
        '--quiet', '-q',
        action='store_true',
        help='Minimal output'
    )

    args = parser.parse_args()

    # Validate arguments
    if args.startup_only and args.pins_only:
        print_error("Cannot use --startup-only and --pins-only together")
        return 1

    if args.vendor and not args.pins_only:
        print_error("--vendor requires --pins-only")
        return 1

    # Execute
    if args.startup_only:
        return generate_startup_only(args)
    elif args.pins_only:
        return generate_pins_only(args)
    else:
        return generate_all(args)


if __name__ == '__main__':
    sys.exit(main())
