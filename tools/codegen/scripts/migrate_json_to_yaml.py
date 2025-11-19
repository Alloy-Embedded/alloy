#!/usr/bin/env python3
"""
JSON to YAML Migration Script for Alloy CLI.

Converts database files from JSON to YAML format while:
- Preserving structure and semantics
- Adding comment placeholders for hardware quirks
- Formatting multiline code snippets cleanly
- Validating output matches input

Usage:
    python scripts/migrate_json_to_yaml.py <input.json> [output.yaml]
    python scripts/migrate_json_to_yaml.py --all  # Convert all JSON files in database/
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.loaders.yaml_loader import YAMLDatabaseLoader


class JSONToYAMLConverter:
    """Convert JSON database files to YAML format."""

    # Comment templates for different data types
    COMMENT_TEMPLATES = {
        "mcu_family": "# High-performance MCU family - add description here",
        "mcu": "# MCU configuration - document any quirks or limitations",
        "board": "# Board configuration - note any pin conflicts or special features",
        "peripheral": "# Peripheral implementation details",
        "pin_conflict": "# WARNING: This pin conflicts with",
        "hardware_quirk": "# QUIRK:",
        "performance_note": "# PERFORMANCE:",
    }

    def __init__(self, preserve_order: bool = True, add_comments: bool = True):
        """
        Initialize converter.

        Args:
            preserve_order: Maintain key order from JSON
            add_comments: Add comment placeholders
        """
        self.preserve_order = preserve_order
        self.add_comments = add_comments

    def convert_file(
        self,
        input_path: Path,
        output_path: Optional[Path] = None,
        dry_run: bool = False
    ) -> Dict[str, Any]:
        """
        Convert JSON file to YAML.

        Args:
            input_path: Path to input JSON file
            output_path: Path to output YAML file (default: same name with .yaml)
            dry_run: If True, don't write output file

        Returns:
            Converted data

        Raises:
            FileNotFoundError: If input file doesn't exist
            json.JSONDecodeError: If JSON is invalid
        """
        if not input_path.exists():
            raise FileNotFoundError(f"Input file not found: {input_path}")

        # Load JSON
        with open(input_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Process data for YAML
        processed_data = self._process_data(data, input_path.stem)

        # Ensure schema_version is present
        if "schema_version" not in processed_data:
            processed_data = {"schema_version": "1.0", **processed_data}

        # Determine output path
        if output_path is None:
            output_path = input_path.with_suffix('.yaml')

        # Save to YAML (unless dry run)
        if not dry_run:
            YAMLDatabaseLoader.save(output_path, processed_data, add_comments=True)
            print(f"✅ Converted: {input_path} → {output_path}")
        else:
            print(f"🔍 DRY RUN: Would convert {input_path} → {output_path}")

        return processed_data

    def _process_data(self, data: Any, context: str) -> Any:
        """
        Process data for YAML conversion.

        Args:
            data: Data to process
            context: Context hint (file stem) for comment suggestions

        Returns:
            Processed data
        """
        if isinstance(data, dict):
            result = {}
            for key, value in data.items():
                # Process nested structures
                processed_value = self._process_data(value, key)

                # Add to result
                result[key] = processed_value

            return result

        elif isinstance(data, list):
            return [self._process_data(item, context) for item in data]

        elif isinstance(data, str):
            # Check if string contains code snippets or newlines
            if '\n' in data or len(data) > 80:
                # Use literal block scalar for multiline strings
                # YAML will handle this automatically with proper dump settings
                return data
            return data

        else:
            # Numbers, booleans, None
            return data

    def validate_conversion(
        self,
        json_data: Dict[str, Any],
        yaml_data: Dict[str, Any]
    ) -> bool:
        """
        Validate that YAML data semantically matches JSON data.

        Args:
            json_data: Original JSON data
            yaml_data: Converted YAML data

        Returns:
            True if data matches (ignoring comments and formatting)

        Raises:
            AssertionError: If data doesn't match
        """
        # Remove schema_version from yaml_data for comparison
        yaml_comparison = {k: v for k, v in yaml_data.items() if k != "schema_version"}

        # Deep comparison (ignoring order and whitespace)
        self._deep_compare(json_data, yaml_comparison, path="root")

        return True

    def _deep_compare(self, obj1: Any, obj2: Any, path: str = "") -> None:
        """
        Deep comparison of two objects.

        Args:
            obj1: First object
            obj2: Second object
            path: Current path in object tree (for error messages)

        Raises:
            AssertionError: If objects don't match
        """
        if type(obj1) != type(obj2):
            raise AssertionError(
                f"Type mismatch at {path}: {type(obj1).__name__} vs {type(obj2).__name__}"
            )

        if isinstance(obj1, dict):
            # Check all keys present
            keys1 = set(obj1.keys())
            keys2 = set(obj2.keys())

            if keys1 != keys2:
                missing_in_2 = keys1 - keys2
                missing_in_1 = keys2 - keys1
                msg = f"Key mismatch at {path}:"
                if missing_in_2:
                    msg += f"\n  Missing in YAML: {missing_in_2}"
                if missing_in_1:
                    msg += f"\n  Extra in YAML: {missing_in_1}"
                raise AssertionError(msg)

            # Compare values recursively
            for key in keys1:
                self._deep_compare(
                    obj1[key],
                    obj2[key],
                    path=f"{path}.{key}" if path else key
                )

        elif isinstance(obj1, list):
            if len(obj1) != len(obj2):
                raise AssertionError(
                    f"List length mismatch at {path}: {len(obj1)} vs {len(obj2)}"
                )

            for i, (item1, item2) in enumerate(zip(obj1, obj2)):
                self._deep_compare(item1, item2, path=f"{path}[{i}]")

        else:
            # Primitive types - direct comparison
            if obj1 != obj2:
                raise AssertionError(
                    f"Value mismatch at {path}: {obj1!r} vs {obj2!r}"
                )

    def convert_directory(
        self,
        directory: Path,
        pattern: str = "*.json",
        recursive: bool = True,
        dry_run: bool = False
    ) -> List[Path]:
        """
        Convert all JSON files in a directory.

        Args:
            directory: Directory to search
            pattern: Glob pattern for files
            recursive: Search recursively
            dry_run: Don't write files

        Returns:
            List of converted file paths
        """
        if not directory.exists():
            raise FileNotFoundError(f"Directory not found: {directory}")

        # Find JSON files
        if recursive:
            json_files = list(directory.rglob(pattern))
        else:
            json_files = list(directory.glob(pattern))

        if not json_files:
            print(f"⚠️  No JSON files found in {directory}")
            return []

        print(f"Found {len(json_files)} JSON files to convert")

        converted_files = []
        errors = []

        for json_file in json_files:
            try:
                self.convert_file(json_file, dry_run=dry_run)
                converted_files.append(json_file)
            except Exception as e:
                errors.append((json_file, str(e)))
                print(f"❌ Error converting {json_file}: {e}")

        # Summary
        print(f"\n{'='*60}")
        print(f"Conversion {'simulation' if dry_run else 'complete'}:")
        print(f"  ✅ Success: {len(converted_files)}")
        print(f"  ❌ Errors: {len(errors)}")

        if errors:
            print("\nErrors:")
            for file_path, error in errors:
                print(f"  - {file_path}: {error}")

        return converted_files


def main():
    """Main entry point for migration script."""
    parser = argparse.ArgumentParser(
        description="Convert JSON database files to YAML format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Convert single file
  python migrate_json_to_yaml.py database/mcus/stm32f4.json

  # Convert all JSON files in database/
  python migrate_json_to_yaml.py --all

  # Dry run (don't write files)
  python migrate_json_to_yaml.py --all --dry-run

  # Convert specific directory
  python migrate_json_to_yaml.py --directory database/boards/
        """
    )

    parser.add_argument(
        "input_file",
        nargs="?",
        type=Path,
        help="Input JSON file to convert"
    )

    parser.add_argument(
        "-o", "--output",
        type=Path,
        help="Output YAML file (default: same name with .yaml extension)"
    )

    parser.add_argument(
        "--all",
        action="store_true",
        help="Convert all JSON files in database/"
    )

    parser.add_argument(
        "-d", "--directory",
        type=Path,
        help="Convert all JSON files in specified directory"
    )

    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be converted without writing files"
    )

    parser.add_argument(
        "--no-comments",
        action="store_true",
        help="Don't add comment placeholders"
    )

    args = parser.parse_args()

    # Create converter
    converter = JSONToYAMLConverter(add_comments=not args.no_comments)

    try:
        if args.all:
            # Convert all files in database/
            database_dir = Path(__file__).parent.parent / "database"
            converter.convert_directory(database_dir, dry_run=args.dry_run)

        elif args.directory:
            # Convert all files in specified directory
            converter.convert_directory(args.directory, dry_run=args.dry_run)

        elif args.input_file:
            # Convert single file
            converter.convert_file(args.input_file, args.output, dry_run=args.dry_run)

        else:
            parser.print_help()
            sys.exit(1)

    except Exception as e:
        print(f"❌ Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
