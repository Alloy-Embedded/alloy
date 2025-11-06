"""
Clean command for removing generated files

This command safely removes generated files tracked in the manifest.
"""

import sys
from pathlib import Path

# Add parent directory to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, COLORS
from cli.core.config import HAL_VENDORS_DIR
from cli.core.manifest import ManifestManager


def execute(args):
    """Execute the clean command"""

    manifest_manager = ManifestManager(HAL_VENDORS_DIR)

    # Check if manifest exists
    if not manifest_manager.manifest_exists():
        print_error("No manifest found. Nothing to clean.")
        print_info("Run './codegen generate' to create files first.")
        return 1

    # Load manifest
    manifest = manifest_manager.load_manifest()

    # Show statistics
    if args.stats:
        return show_statistics(manifest_manager)

    # Validate files
    if args.validate:
        return validate_files(manifest_manager)

    # Clean files
    if args.generator:
        return clean_generator(manifest_manager, args.generator, args.dry_run)
    else:
        return clean_all(manifest_manager, args.dry_run)


def show_statistics(manifest_manager: ManifestManager) -> int:
    """Show manifest statistics"""
    print_header("Manifest Statistics")

    stats = manifest_manager.get_statistics()

    print(f"\n📊 Overview:")
    print(f"  Total files tracked: {stats['total_files']}")
    print(f"  Total size: {_format_size(stats['total_size'])}")
    print(f"  Last updated: {stats['last_updated']}")

    if stats['generators']:
        print(f"\n🔨 By generator:")
        for gen_id, gen_stats in sorted(stats['generators'].items()):
            print(f"  {gen_id:20s}: {gen_stats['count']:3d} files ({_format_size(gen_stats['size'])})")
            print(f"  {'':20s}  Last run: {gen_stats['last_run']}")

    print(f"\n💾 Manifest: {manifest_manager.manifest_path}\n")

    return 0


def validate_files(manifest_manager: ManifestManager) -> int:
    """Validate all files against manifest"""
    print_header("Validating Files")

    all_files = manifest_manager.get_all_files()
    valid_count = 0
    invalid_count = 0
    missing_count = 0

    print()
    for entry in all_files:
        file_path = HAL_VENDORS_DIR / entry.path

        if not file_path.exists():
            print(f"  [{COLORS['YELLOW']}⚠️{COLORS['RESET']}] {entry.path}: File missing")
            missing_count += 1
            continue

        is_valid, error = manifest_manager.validate_file(file_path)

        if is_valid:
            if sys.stdout.isatty():  # Only show success if terminal (not piped)
                print(f"  [{COLORS['GREEN']}✅{COLORS['RESET']}] {entry.path}")
            valid_count += 1
        else:
            print(f"  [{COLORS['RED']}❌{COLORS['RESET']}] {entry.path}: {error}")
            invalid_count += 1

    # Summary
    print()
    print(f"Valid: {valid_count}")
    if missing_count > 0:
        print(f"{COLORS['YELLOW']}Missing: {missing_count}{COLORS['RESET']}")
    if invalid_count > 0:
        print(f"{COLORS['RED']}Invalid: {invalid_count}{COLORS['RESET']}")

    print()
    if invalid_count > 0:
        print_error("Some files were modified! Regenerate them with './codegen generate'")
        return 1
    elif missing_count > 0:
        print_info("Some files are missing. Regenerate with './codegen generate'")
        return 1
    else:
        print_success("All files are valid!")
        return 0


def clean_generator(manifest_manager: ManifestManager, generator: str, dry_run: bool) -> int:
    """Clean files from a specific generator"""

    if dry_run:
        print_header(f"Clean Files: {generator} (DRY RUN)")
    else:
        print_header(f"Clean Files: {generator}")

    # Get files to delete
    files = manifest_manager.get_files_by_generator(generator)

    if not files:
        print_error(f"No files found for generator: {generator}")
        print_info(f"Available generators: {list(manifest_manager.load_manifest()['generators'].keys())}")
        return 1

    print(f"\nFiles to delete from generator '{generator}':\n")

    total_size = 0
    for entry in files:
        file_path = HAL_VENDORS_DIR / entry.path
        exists = "✓" if file_path.exists() else "✗"
        size = _format_size(entry.size)
        print(f"  [{exists}] {entry.path} ({size})")
        total_size += entry.size

    print(f"\nTotal: {len(files)} files ({_format_size(total_size)})")

    if dry_run:
        print_info("\n⚠️  This is a DRY RUN. No files were deleted.")
        print_info("Run without --dry-run to actually delete files.")
        return 0

    # Confirm deletion
    print()
    response = input(f"{COLORS['YELLOW']}Delete {len(files)} files? (yes/no): {COLORS['RESET']}")

    if response.lower() not in ['yes', 'y']:
        print_info("Deletion cancelled.")
        return 0

    # Delete files
    deleted, errors = manifest_manager.clean_generator(generator, dry_run=False)

    print()
    if deleted:
        print_success(f"Deleted {len(deleted)} files")

    if errors:
        print_error(f"{len(errors)} errors occurred:")
        for error in errors:
            print(f"  - {error}")
        return 1

    return 0


def clean_all(manifest_manager: ManifestManager, dry_run: bool) -> int:
    """Clean all generated files"""

    if dry_run:
        print_header("Clean All Files (DRY RUN)")
    else:
        print_header("Clean All Files")

    # Get all files
    all_files = manifest_manager.get_all_files()

    if not all_files:
        print_info("No files to clean.")
        return 0

    print(f"\nAll generated files:\n")

    total_size = 0
    by_generator = {}

    for entry in all_files:
        file_path = HAL_VENDORS_DIR / entry.path
        exists = "✓" if file_path.exists() else "✗"

        if entry.generator not in by_generator:
            by_generator[entry.generator] = []

        by_generator[entry.generator].append(entry)
        total_size += entry.size

    # Show by generator
    for generator, files in sorted(by_generator.items()):
        gen_size = sum(f.size for f in files)
        print(f"  {generator}: {len(files)} files ({_format_size(gen_size)})")

    print(f"\nTotal: {len(all_files)} files ({_format_size(total_size)})")

    if dry_run:
        print_info("\n⚠️  This is a DRY RUN. No files were deleted.")
        print_info("Run without --dry-run to actually delete files.")
        return 0

    # Confirm deletion
    print()
    print(f"{COLORS['RED']}WARNING: This will delete ALL generated files!{COLORS['RESET']}")
    response = input(f"{COLORS['YELLOW']}Are you sure? Type 'yes' to confirm: {COLORS['RESET']}")

    if response.lower() != 'yes':
        print_info("Deletion cancelled.")
        return 0

    # Delete files
    deleted, errors = manifest_manager.clean_all(dry_run=False)

    print()
    if deleted:
        print_success(f"Deleted {len(deleted)} files")

    if errors:
        print_error(f"{len(errors)} errors occurred:")
        for error in errors:
            print(f"  - {error}")
        return 1

    return 0


def _format_size(size_bytes: int) -> str:
    """Format file size in human-readable format"""
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.1f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.1f} MB"


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Clean generated files')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be deleted')
    parser.add_argument('--stats', action='store_true', help='Show manifest statistics')
    parser.add_argument('--validate', action='store_true', help='Validate file checksums')
    parser.add_argument('--generator', help='Clean files from specific generator only')

    args = parser.parse_args()
    sys.exit(execute(args))
