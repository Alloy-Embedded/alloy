"""
Generic Database Loader with automatic format detection.

Supports both JSON and YAML formats with seamless transition.
"""

import json
from pathlib import Path
from typing import Any, Dict, Optional
from .yaml_loader import YAMLDatabaseLoader


class DatabaseLoader:
    """
    Generic database loader with format auto-detection.

    Automatically detects whether a file is JSON or YAML based on extension,
    allowing gradual migration from JSON to YAML.
    """

    @staticmethod
    def load(file_path: Path) -> Dict[str, Any]:
        """
        Load database file (JSON or YAML) with automatic format detection.

        Args:
            file_path: Path to database file (.json or .yaml/.yml)

        Returns:
            Parsed data as dictionary

        Raises:
            FileNotFoundError: If file doesn't exist
            ValueError: If file format is unsupported
            Exception: If parsing fails
        """
        if not file_path.exists():
            raise FileNotFoundError(f"Database file not found: {file_path}")

        # Detect format by extension
        suffix = file_path.suffix.lower()

        if suffix in ['.yaml', '.yml']:
            return YAMLDatabaseLoader.load(file_path)
        elif suffix == '.json':
            return DatabaseLoader._load_json(file_path)
        else:
            raise ValueError(
                f"Unsupported file format: {suffix}. "
                "Use .json, .yaml, or .yml"
            )

    @staticmethod
    def _load_json(file_path: Path) -> Dict[str, Any]:
        """
        Load JSON file.

        Args:
            file_path: Path to JSON file

        Returns:
            Parsed JSON data

        Raises:
            json.JSONDecodeError: If JSON is malformed
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                data = json.load(f)

            return data

        except json.JSONDecodeError as e:
            error_msg = (
                f"JSON syntax error in {file_path}:\n"
                f"  Line {e.lineno}, Column {e.colno}\n"
                f"  {e.msg}"
            )
            raise json.JSONDecodeError(error_msg, e.doc, e.pos) from e

    @staticmethod
    def save(file_path: Path, data: Dict[str, Any], format: Optional[str] = None) -> None:
        """
        Save data to file (format detected by extension or explicit format).

        Args:
            file_path: Path to save file
            data: Data to save
            format: Optional explicit format ('json' or 'yaml')

        Raises:
            ValueError: If format is unsupported
        """
        # Determine format
        if format:
            fmt = format.lower()
        else:
            suffix = file_path.suffix.lower()
            if suffix in ['.yaml', '.yml']:
                fmt = 'yaml'
            elif suffix == '.json':
                fmt = 'json'
            else:
                raise ValueError(
                    f"Cannot determine format from extension: {suffix}. "
                    "Specify format explicitly or use .json/.yaml extension"
                )

        # Save based on format
        if fmt == 'yaml':
            YAMLDatabaseLoader.save(file_path, data, add_comments=True)
        elif fmt == 'json':
            file_path.parent.mkdir(parents=True, exist_ok=True)
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
        else:
            raise ValueError(f"Unsupported format: {fmt}")


def auto_detect_loader(file_path: Path) -> Dict[str, Any]:
    """
    Convenience function for loading database files with auto-detection.

    Args:
        file_path: Path to database file

    Returns:
        Parsed data

    Example:
        >>> data = auto_detect_loader(Path("database/mcus/stm32f4.yaml"))
        >>> data = auto_detect_loader(Path("database/mcus/same70.json"))
    """
    return DatabaseLoader.load(file_path)


def find_database_file(base_path: Path, name: str) -> Optional[Path]:
    """
    Find database file by name, trying both YAML and JSON extensions.

    Prefers YAML over JSON if both exist.

    Args:
        base_path: Directory to search
        name: File name without extension

    Returns:
        Path to found file, or None if not found

    Example:
        >>> file = find_database_file(Path("database/mcus"), "stm32f4")
        >>> # Returns stm32f4.yaml if exists, else stm32f4.json
    """
    # Try YAML first (preferred format)
    yaml_path = base_path / f"{name}.yaml"
    if yaml_path.exists():
        return yaml_path

    yml_path = base_path / f"{name}.yml"
    if yml_path.exists():
        return yml_path

    # Fallback to JSON
    json_path = base_path / f"{name}.json"
    if json_path.exists():
        return json_path

    return None
