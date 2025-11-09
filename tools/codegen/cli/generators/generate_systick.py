"""
Generate SysTick HAL from templates and metadata

This module generates platform-specific SysTick implementations using
Jinja2 templates and JSON metadata files.

Usage:
    python3 generate_systick.py --platform same70
    python3 generate_systick.py --all
"""

import sys
import json
from pathlib import Path
from typing import Optional

# Add parent to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, logger
from cli.core.config import CODEGEN_DIR, REPO_ROOT
from cli.core.paths import ensure_dir

from jinja2 import Environment, FileSystemLoader

# Define directories
METADATA_PLATFORM_DIR = CODEGEN_DIR / "cli" / "generators" / "metadata" / "platform"
TEMPLATES_PLATFORM_DIR = CODEGEN_DIR / "templates" / "platform"
HAL_PLATFORM_DIR = REPO_ROOT / "src" / "hal" / "platform"


def generate_systick_for_platform(platform: str, verbose: bool = False) -> bool:
    """
    Generate SysTick HAL for a specific platform.

    Args:
        platform: Platform name (e.g., 'same70')
        verbose: Enable verbose output

    Returns:
        True if generation succeeded
    """
    # Load metadata
    metadata_file = METADATA_PLATFORM_DIR / f"{platform}_systick.json"
    if not metadata_file.exists():
        if verbose:
            print_error(f"No SysTick metadata for platform: {platform}")
        return False

    try:
        with open(metadata_file, 'r') as f:
            metadata = json.load(f)
    except Exception as e:
        logger.error(f"Failed to load metadata: {e}")
        return False

    # Get output directory
    output_dir = ensure_dir(HAL_PLATFORM_DIR / metadata['family'])
    output_file = output_dir / "systick.hpp"

    # Render template
    try:
        env = Environment(loader=FileSystemLoader(CODEGEN_DIR / "templates"))
        template = env.get_template("platform/systick.hpp.j2")
        content = template.render(metadata=metadata)
        output_file.write_text(content)

        if verbose:
            print_success(f"Generated: {output_file}")

        logger.info(f"Generated SysTick HAL for {platform}")
        return True

    except Exception as e:
        logger.error(f"Failed to generate SysTick for {platform}: {e}")
        return False


def generate_all_systick(verbose: bool = False) -> int:
    """
    Generate SysTick HAL for all platforms with metadata.

    Args:
        verbose: Enable verbose output

    Returns:
        Exit code (0 for success)
    """
    print_header("Generating SysTick HAL")

    # Find all systick metadata files
    metadata_files = list(METADATA_PLATFORM_DIR.glob("*_systick.json"))

    if not metadata_files:
        print_error("No SysTick metadata files found!")
        return 1

    print_info(f"Found {len(metadata_files)} platform(s) with SysTick metadata")

    success_count = 0
    for metadata_file in metadata_files:
        # Extract platform name (e.g., "same70_systick.json" -> "same70")
        platform = metadata_file.stem.replace('_systick', '')

        if generate_systick_for_platform(platform, verbose):
            success_count += 1

    print_success(f"Generated SysTick HAL for {success_count}/{len(metadata_files)} platform(s)")
    return 0 if success_count == len(metadata_files) else 1


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description='Generate SysTick HAL from templates')
    parser.add_argument('--platform', help='Generate for specific platform (e.g., same70)')
    parser.add_argument('--all', action='store_true', help='Generate for all platforms')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    args = parser.parse_args()

    if args.platform:
        success = generate_systick_for_platform(args.platform, args.verbose)
        return 0 if success else 1
    elif args.all:
        return generate_all_systick(args.verbose)
    else:
        print_error("Please specify --platform or --all")
        parser.print_help()
        return 1


if __name__ == '__main__':
    sys.exit(main())
