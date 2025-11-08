#!/usr/bin/env python3
"""
Regenerate ALL startup.cpp files from their corresponding SVD files.

This script finds all existing startup.cpp files, locates their corresponding
SVD files, and regenerates them with the fixed template.
"""

import sys
from pathlib import Path
import subprocess
import re

# Add codegen to path
CODEGEN_DIR = Path(__file__).parent.parent
COREZERO_ROOT = CODEGEN_DIR.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info
from cli.parsers.svd_discovery import discover_all_svds, find_svd_for_mcu

def extract_mcu_name_from_path(startup_path: Path) -> str:
    """Extract MCU name from startup.cpp path.

    Path structure: src/hal/vendors/{vendor}/{family}/{mcu}/startup.cpp
    """
    return startup_path.parent.name

def main():
    print_header("Regenerating ALL startup.cpp files")

    # Find all existing startup.cpp files
    startup_files = list(COREZERO_ROOT.glob("src/hal/vendors/**/startup.cpp"))
    print_info(f"Found {len(startup_files)} startup.cpp files")

    # Discover all available SVD files
    print_info("Discovering SVD files...")
    all_svds = discover_all_svds()
    print_info(f"Found {len(all_svds)} SVD files")

    # Match each startup.cpp to its SVD
    regenerated = 0
    skipped = 0
    failed = 0

    for startup_path in startup_files:
        mcu_name = extract_mcu_name_from_path(startup_path)

        # Find corresponding SVD
        svd_info = find_svd_for_mcu(mcu_name, all_svds)

        if not svd_info:
            print_info(f"⚠️  No SVD found for {mcu_name} - skipping")
            skipped += 1
            continue

        print_info(f"🔄 Regenerating {mcu_name}...")

        # Run the generator
        try:
            result = subprocess.run(
                [
                    sys.executable,
                    str(CODEGEN_DIR / "cli" / "generators" / "generate_startup.py"),
                    "--svd", str(svd_info.file_path)
                ],
                capture_output=True,
                text=True,
                timeout=30,
                cwd=CODEGEN_DIR
            )

            if result.returncode == 0:
                regenerated += 1
                print_success(f"✅ {mcu_name}")
            else:
                failed += 1
                print_error(f"❌ {mcu_name}: {result.stderr[:100]}")
        except Exception as e:
            failed += 1
            print_error(f"❌ {mcu_name}: {e}")

    # Summary
    print("")
    print_header("Regeneration Summary")
    print_success(f"✅ Regenerated: {regenerated} files")
    if skipped > 0:
        print_info(f"⚠️  Skipped: {skipped} files (no SVD)")
    if failed > 0:
        print_error(f"❌ Failed: {failed} files")

    print("")
    print_info(f"Total: {len(startup_files)} files")

    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
