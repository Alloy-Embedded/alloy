#!/usr/bin/env python3
"""
Alloy SVD Synchronization Tool

Downloads and organizes SVD files from ARM CMSIS-Pack repository.
Keeps SVDs up-to-date and organized by vendor for easy access.

Usage:
    python sync_svd.py --init              # Initial setup
    python sync_svd.py --update            # Update from upstream
    python sync_svd.py --list-vendors      # List available vendors
    python sync_svd.py --list-mcus STM32   # List MCUs for vendor
"""

import argparse
import subprocess
import sys
from pathlib import Path
from typing import List, Dict, Optional
import json

# Paths
SCRIPT_DIR = Path(__file__).parent
UPSTREAM_DIR = SCRIPT_DIR / "upstream"
CMSIS_SVD_DIR = UPSTREAM_DIR / "cmsis-svd-data"
DATABASE_SVD_DIR = SCRIPT_DIR / "database" / "svd"
INDEX_FILE = DATABASE_SVD_DIR / "INDEX.md"

# CMSIS-SVD repository
CMSIS_SVD_REPO = "https://github.com/cmsis-svd/cmsis-svd-data.git"
CMSIS_SVD_DATA_DIR = "data"

class Colors:
    """ANSI color codes for terminal output"""
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'


def print_header(msg: str):
    """Print colored header message"""
    print(f"{Colors.HEADER}{Colors.BOLD}{msg}{Colors.ENDC}")


def print_success(msg: str):
    """Print colored success message"""
    print(f"{Colors.OKGREEN}✓{Colors.ENDC} {msg}")


def print_info(msg: str):
    """Print colored info message"""
    print(f"{Colors.OKCYAN}→{Colors.ENDC} {msg}")


def print_warning(msg: str):
    """Print colored warning message"""
    print(f"{Colors.WARNING}⚠{Colors.ENDC} {msg}")


def print_error(msg: str):
    """Print colored error message"""
    print(f"{Colors.FAIL}✗{Colors.ENDC} {msg}", file=sys.stderr)


def run_command(cmd: List[str], cwd: Optional[Path] = None, check: bool = True) -> subprocess.CompletedProcess:
    """Run shell command and return result"""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            check=check,
            capture_output=True,
            text=True
        )
        return result
    except subprocess.CalledProcessError as e:
        print_error(f"Command failed: {' '.join(cmd)}")
        print_error(f"Error: {e.stderr}")
        if check:
            sys.exit(1)
        return e


def init_submodule(dry_run: bool = False) -> bool:
    """Initialize CMSIS-SVD git submodule"""
    print_header("Initializing CMSIS-SVD submodule...")

    # Check if git is available
    result = run_command(["git", "--version"], check=False)
    if result.returncode != 0:
        print_error("Git is not installed or not in PATH")
        return False

    # Check if we're in a git repository
    result = run_command(["git", "rev-parse", "--git-dir"], check=False)
    if result.returncode != 0:
        print_error("Not in a git repository")
        return False

    if dry_run:
        print_info("[DRY RUN] Would initialize git submodule at tools/codegen/upstream/cmsis-svd-data")
        return True

    # Check if submodule already exists
    if CMSIS_SVD_DIR.exists() and (CMSIS_SVD_DIR / ".git").exists():
        print_success("CMSIS-SVD submodule already initialized")
        return True

    # Create upstream directory if it doesn't exist
    UPSTREAM_DIR.mkdir(parents=True, exist_ok=True)

    # Add submodule if not already in .gitmodules
    print_info("Adding CMSIS-SVD as git submodule...")
    result = run_command([
        "git", "submodule", "add", "--depth", "1",
        CMSIS_SVD_REPO,
        str(CMSIS_SVD_DIR.relative_to(Path.cwd()))
    ], check=False)

    if result.returncode != 0 and "already exists" not in result.stderr:
        # If failed and not because it already exists, try update instead
        print_info("Trying to initialize existing submodule...")
        result = run_command([
            "git", "submodule", "update", "--init", "--depth", "1",
            str(CMSIS_SVD_DIR.relative_to(Path.cwd()))
        ], check=False)

    if result.returncode == 0 or CMSIS_SVD_DIR.exists():
        print_success(f"CMSIS-SVD submodule initialized at {CMSIS_SVD_DIR}")
        return True
    else:
        print_error("Failed to initialize submodule")
        print_error(f"Error: {result.stderr}")
        return False


def update_submodule(dry_run: bool = False) -> bool:
    """Update CMSIS-SVD submodule to latest"""
    print_header("Updating CMSIS-SVD submodule...")

    if not CMSIS_SVD_DIR.exists():
        print_warning("Submodule not initialized. Run --init first.")
        return False

    if dry_run:
        print_info("[DRY RUN] Would update git submodule to latest commit")
        return True

    print_info("Pulling latest changes from CMSIS-SVD...")
    result = run_command([
        "git", "submodule", "update", "--remote", "--depth", "1",
        "tools/codegen/upstream/cmsis-svd"
    ])

    if result.returncode == 0:
        print_success("CMSIS-SVD submodule updated")
        return True
    else:
        print_error("Failed to update submodule")
        return False


def get_available_vendors() -> Dict[str, int]:
    """Get list of available vendors and MCU counts"""
    vendors = {}

    if not CMSIS_SVD_DIR.exists():
        return vendors

    data_dir = CMSIS_SVD_DIR / CMSIS_SVD_DATA_DIR

    if not data_dir.exists():
        return vendors

    for vendor_dir in data_dir.iterdir():
        if vendor_dir.is_dir() and not vendor_dir.name.startswith('.'):
            svd_files = list(vendor_dir.glob("*.svd"))
            if svd_files:
                vendors[vendor_dir.name] = len(svd_files)

    return vendors


def list_vendors():
    """List all available vendors"""
    print_header("Available SVD Vendors:")

    vendors = get_available_vendors()

    if not vendors:
        print_warning("No vendors found. Run --init first.")
        return

    # Sort by name
    for vendor, count in sorted(vendors.items()):
        print(f"  {Colors.OKBLUE}{vendor}{Colors.ENDC}: {count} MCUs")

    print(f"\nTotal: {len(vendors)} vendors, {sum(vendors.values())} MCUs")


def list_mcus(vendor: str):
    """List all MCUs for a specific vendor"""
    print_header(f"MCUs for vendor: {vendor}")

    data_dir = CMSIS_SVD_DIR / CMSIS_SVD_DATA_DIR / vendor

    if not data_dir.exists():
        print_error(f"Vendor '{vendor}' not found")
        return

    svd_files = sorted(data_dir.glob("*.svd"))

    if not svd_files:
        print_warning(f"No SVD files found for {vendor}")
        return

    for svd_file in svd_files:
        mcu_name = svd_file.stem
        file_size = svd_file.stat().st_size / 1024  # KB
        print(f"  {Colors.OKBLUE}{mcu_name}{Colors.ENDC} ({file_size:.1f} KB)")

    print(f"\nTotal: {len(svd_files)} MCUs")


def organize_svds(vendors: Optional[List[str]] = None, dry_run: bool = False) -> bool:
    """Organize SVD files by creating symlinks"""
    print_header("Organizing SVD files...")

    if not CMSIS_SVD_DIR.exists():
        print_error("CMSIS-SVD not initialized. Run --init first.")
        return False

    data_dir = CMSIS_SVD_DIR / CMSIS_SVD_DATA_DIR

    if not data_dir.exists():
        print_error(f"Data directory not found: {data_dir}")
        return False

    # Create database/svd directory
    if not dry_run:
        DATABASE_SVD_DIR.mkdir(parents=True, exist_ok=True)

    all_vendors = get_available_vendors()

    # Filter vendors if specified
    if vendors:
        filtered_vendors = {v: all_vendors[v] for v in vendors if v in all_vendors}
        if not filtered_vendors:
            print_error(f"None of the specified vendors found: {vendors}")
            return False
        target_vendors = filtered_vendors
    else:
        target_vendors = all_vendors

    print_info(f"Organizing {len(target_vendors)} vendors...")

    for vendor in sorted(target_vendors.keys()):
        vendor_link = DATABASE_SVD_DIR / vendor
        vendor_source = data_dir / vendor

        if dry_run:
            print_info(f"[DRY RUN] Would create: {vendor_link} -> {vendor_source}")
        else:
            # Remove old symlink if exists
            if vendor_link.exists() or vendor_link.is_symlink():
                vendor_link.unlink()

            # Create symlink
            try:
                vendor_link.symlink_to(vendor_source.relative_to(DATABASE_SVD_DIR), target_is_directory=True)
                print_success(f"{vendor}: {target_vendors[vendor]} MCUs")
            except Exception as e:
                print_error(f"Failed to create symlink for {vendor}: {e}")

    return True


def generate_index(dry_run: bool = False):
    """Generate INDEX.md with all available SVDs"""
    print_header("Generating INDEX.md...")

    vendors = get_available_vendors()

    if not vendors:
        print_warning("No vendors to index")
        return

    # Build index content
    lines = [
        "# SVD File Index",
        "",
        "Auto-generated index of available SVD files.",
        "",
        f"**Total:** {len(vendors)} vendors, {sum(vendors.values())} MCUs",
        "",
        "## Vendors",
        ""
    ]

    for vendor, count in sorted(vendors.items()):
        lines.append(f"### {vendor} ({count} MCUs)")
        lines.append("")

        data_dir = CMSIS_SVD_DIR / CMSIS_SVD_DATA_DIR / vendor
        if data_dir.exists():
            svd_files = sorted(data_dir.glob("*.svd"))
            for svd_file in svd_files:
                mcu_name = svd_file.stem
                rel_path = svd_file.relative_to(CMSIS_SVD_DIR)
                lines.append(f"- `{mcu_name}` - `{vendor}/{svd_file.name}`")

        lines.append("")

    lines.append("---")
    lines.append("")
    lines.append("*Generated by sync_svd.py*")
    lines.append("")

    content = "\n".join(lines)

    if dry_run:
        print_info(f"[DRY RUN] Would write INDEX.md ({len(lines)} lines)")
        return

    INDEX_FILE.write_text(content)
    print_success(f"INDEX.md generated ({len(lines)} lines)")


def main():
    parser = argparse.ArgumentParser(
        description="Synchronize and organize CMSIS-SVD files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python sync_svd.py --init                    # Initial setup
  python sync_svd.py --update                  # Update from upstream
  python sync_svd.py --update --vendor STM32   # Update only STM32
  python sync_svd.py --list-vendors            # List all vendors
  python sync_svd.py --list-mcus STM32         # List STM32 MCUs
  python sync_svd.py --dry-run --update        # Preview changes
        """
    )

    parser.add_argument('--init', action='store_true',
                        help='Initialize CMSIS-SVD submodule')
    parser.add_argument('--update', action='store_true',
                        help='Update CMSIS-SVD to latest')
    parser.add_argument('--vendor', type=str,
                        help='Comma-separated list of vendors to sync (e.g., STM32,nRF)')
    parser.add_argument('--list-vendors', action='store_true',
                        help='List all available vendors')
    parser.add_argument('--list-mcus', type=str, metavar='VENDOR',
                        help='List all MCUs for a vendor')
    parser.add_argument('--dry-run', action='store_true',
                        help='Preview changes without executing')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')

    args = parser.parse_args()

    # Handle list commands
    if args.list_vendors:
        list_vendors()
        return 0

    if args.list_mcus:
        list_mcus(args.list_mcus)
        return 0

    # Main workflow
    if args.init:
        if not init_submodule(dry_run=args.dry_run):
            return 1

    if args.update:
        if not update_submodule(dry_run=args.dry_run):
            return 1

    if args.init or args.update:
        # Parse vendor list
        vendors = None
        if args.vendor:
            vendors = [v.strip() for v in args.vendor.split(',')]
            print_info(f"Filtering vendors: {', '.join(vendors)}")

        # Organize SVDs
        if not organize_svds(vendors=vendors, dry_run=args.dry_run):
            return 1

        # Generate index
        generate_index(dry_run=args.dry_run)

        if not args.dry_run:
            print()
            print_success("SVD synchronization complete!")
            print_info("Use --list-vendors to see available vendors")

    if not any([args.init, args.update, args.list_vendors, args.list_mcus]):
        parser.print_help()
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
