"""
Manifest management for tracking generated files

The manifest system tracks all generated files with metadata:
- File path
- Generator that created it
- Timestamp
- File size
- SHA256 checksum

This enables:
- Safe cleanup (only delete tracked files)
- Validation (detect manual modifications)
- Audit trail (who generated what and when)
"""

import json
import hashlib
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, field


@dataclass
class FileEntry:
    """Represents a tracked file in the manifest"""
    path: str
    generator: str
    timestamp: str
    size: int
    checksum: str

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization"""
        return {
            'path': self.path,
            'generator': self.generator,
            'timestamp': self.timestamp,
            'size': self.size,
            'checksum': self.checksum
        }

    @staticmethod
    def from_dict(data: Dict) -> 'FileEntry':
        """Create from dictionary"""
        return FileEntry(
            path=data['path'],
            generator=data['generator'],
            timestamp=data['timestamp'],
            size=data['size'],
            checksum=data['checksum']
        )


class ManifestManager:
    """
    Manages the manifest file that tracks all generated code files.

    The manifest is stored as JSON in the vendors directory:
    src/hal/vendors/.generated_manifest.json

    Format:
    {
        "version": "1.0",
        "last_updated": "2025-11-05T22:30:15",
        "files": {
            "relative/path/to/file.hpp": {
                "path": "relative/path/to/file.hpp",
                "generator": "st_pins",
                "timestamp": "2025-11-05T22:30:15",
                "size": 1234,
                "checksum": "sha256..."
            },
            ...
        },
        "generators": {
            "st_pins": {
                "count": 130,
                "last_run": "2025-11-05T22:30:15"
            },
            ...
        }
    }
    """

    MANIFEST_VERSION = "1.0"
    MANIFEST_FILENAME = ".generated_manifest.json"

    def __init__(self, vendors_dir: Path):
        """
        Initialize manifest manager.

        Args:
            vendors_dir: Path to src/hal/vendors directory
        """
        self.vendors_dir = Path(vendors_dir)
        self.manifest_path = self.vendors_dir / self.MANIFEST_FILENAME
        self._manifest: Optional[Dict] = None

    def manifest_exists(self) -> bool:
        """Check if manifest file exists"""
        return self.manifest_path.exists()

    def load_manifest(self) -> Dict:
        """
        Load manifest from disk.

        Returns:
            Manifest dictionary
        """
        if not self.manifest_exists():
            return self._create_empty_manifest()

        try:
            with open(self.manifest_path, 'r', encoding='utf-8') as f:
                self._manifest = json.load(f)
                return self._manifest
        except Exception as e:
            print(f"Warning: Failed to load manifest: {e}")
            return self._create_empty_manifest()

    def save_manifest(self, manifest: Optional[Dict] = None) -> bool:
        """
        Save manifest to disk.

        Args:
            manifest: Manifest to save (uses internal if None)

        Returns:
            True if saved successfully
        """
        if manifest is None:
            manifest = self._manifest

        if manifest is None:
            manifest = self._create_empty_manifest()

        try:
            # Ensure directory exists
            self.vendors_dir.mkdir(parents=True, exist_ok=True)

            # Update last_updated timestamp
            manifest['last_updated'] = datetime.now().isoformat()

            # Write atomically (write to temp, then rename)
            temp_path = self.manifest_path.with_suffix('.tmp')
            with open(temp_path, 'w', encoding='utf-8') as f:
                json.dump(manifest, f, indent=2, sort_keys=True)

            temp_path.replace(self.manifest_path)

            self._manifest = manifest
            return True

        except Exception as e:
            print(f"Error: Failed to save manifest: {e}")
            return False

    def add_file(self, file_path: Path, generator: str) -> bool:
        """
        Add a file to the manifest.

        Args:
            file_path: Absolute path to the file
            generator: Generator ID (e.g., "st_pins", "startup")

        Returns:
            True if added successfully
        """
        if not file_path.exists():
            print(f"Warning: Cannot add non-existent file: {file_path}")
            return False

        # Load manifest if not loaded
        if self._manifest is None:
            self._manifest = self.load_manifest()

        try:
            # Get relative path from vendors dir
            rel_path = str(file_path.relative_to(self.vendors_dir))

            # Calculate file metadata
            file_size = file_path.stat().st_size
            file_checksum = self._calculate_checksum(file_path)
            timestamp = datetime.now().isoformat()

            # Create file entry
            entry = FileEntry(
                path=rel_path,
                generator=generator,
                timestamp=timestamp,
                size=file_size,
                checksum=file_checksum
            )

            # Add to manifest
            self._manifest['files'][rel_path] = entry.to_dict()

            # Update generator stats
            if generator not in self._manifest['generators']:
                self._manifest['generators'][generator] = {
                    'count': 0,
                    'last_run': timestamp
                }

            self._manifest['generators'][generator]['count'] = len([
                f for f in self._manifest['files'].values()
                if f['generator'] == generator
            ])
            self._manifest['generators'][generator]['last_run'] = timestamp

            return True

        except Exception as e:
            print(f"Error: Failed to add file to manifest: {e}")
            return False

    def remove_file(self, file_path: Path) -> bool:
        """
        Remove a file from the manifest.

        Args:
            file_path: Absolute path to the file

        Returns:
            True if removed successfully
        """
        if self._manifest is None:
            self._manifest = self.load_manifest()

        try:
            rel_path = str(file_path.relative_to(self.vendors_dir))

            if rel_path in self._manifest['files']:
                generator = self._manifest['files'][rel_path]['generator']
                del self._manifest['files'][rel_path]

                # Update generator count
                if generator in self._manifest['generators']:
                    self._manifest['generators'][generator]['count'] = len([
                        f for f in self._manifest['files'].values()
                        if f['generator'] == generator
                    ])

                return True

            return False

        except Exception as e:
            print(f"Error: Failed to remove file from manifest: {e}")
            return False

    def get_files_by_generator(self, generator: str) -> List[FileEntry]:
        """
        Get all files generated by a specific generator.

        Args:
            generator: Generator ID

        Returns:
            List of FileEntry objects
        """
        if self._manifest is None:
            self._manifest = self.load_manifest()

        return [
            FileEntry.from_dict(entry)
            for entry in self._manifest['files'].values()
            if entry['generator'] == generator
        ]

    def get_all_files(self) -> List[FileEntry]:
        """
        Get all tracked files.

        Returns:
            List of FileEntry objects
        """
        if self._manifest is None:
            self._manifest = self.load_manifest()

        return [
            FileEntry.from_dict(entry)
            for entry in self._manifest['files'].values()
        ]

    def validate_file(self, file_path: Path) -> tuple[bool, Optional[str]]:
        """
        Validate a file against its manifest entry.

        Args:
            file_path: Absolute path to the file

        Returns:
            Tuple of (is_valid, error_message)
        """
        if self._manifest is None:
            self._manifest = self.load_manifest()

        try:
            rel_path = str(file_path.relative_to(self.vendors_dir))

            if rel_path not in self._manifest['files']:
                return False, "File not in manifest"

            if not file_path.exists():
                return False, "File does not exist"

            entry = self._manifest['files'][rel_path]

            # Check size
            actual_size = file_path.stat().st_size
            if actual_size != entry['size']:
                return False, f"Size mismatch: expected {entry['size']}, got {actual_size}"

            # Check checksum
            actual_checksum = self._calculate_checksum(file_path)
            if actual_checksum != entry['checksum']:
                return False, "Checksum mismatch (file was modified)"

            return True, None

        except Exception as e:
            return False, f"Validation error: {e}"

    def get_statistics(self) -> Dict:
        """
        Get manifest statistics.

        Returns:
            Dictionary with statistics
        """
        if self._manifest is None:
            self._manifest = self.load_manifest()

        total_files = len(self._manifest['files'])
        total_size = sum(f['size'] for f in self._manifest['files'].values())

        generators = {}
        for gen_id, gen_info in self._manifest['generators'].items():
            files = [f for f in self._manifest['files'].values() if f['generator'] == gen_id]
            generators[gen_id] = {
                'count': len(files),
                'size': sum(f['size'] for f in files),
                'last_run': gen_info['last_run']
            }

        return {
            'total_files': total_files,
            'total_size': total_size,
            'generators': generators,
            'last_updated': self._manifest.get('last_updated', 'Unknown')
        }

    def clean_generator(self, generator: str, dry_run: bool = True) -> tuple[List[Path], List[str]]:
        """
        Clean all files from a specific generator.

        Args:
            generator: Generator ID
            dry_run: If True, don't actually delete files

        Returns:
            Tuple of (deleted_paths, errors)
        """
        files_to_delete = self.get_files_by_generator(generator)
        deleted = []
        errors = []

        for entry in files_to_delete:
            file_path = self.vendors_dir / entry.path

            try:
                if file_path.exists():
                    if not dry_run:
                        file_path.unlink()
                        self.remove_file(file_path)
                    deleted.append(file_path)
                else:
                    errors.append(f"File not found: {file_path}")

            except Exception as e:
                errors.append(f"Failed to delete {file_path}: {e}")

        if not dry_run and deleted:
            self.save_manifest()

        return deleted, errors

    def clean_all(self, dry_run: bool = True) -> tuple[List[Path], List[str]]:
        """
        Clean all tracked files.

        Args:
            dry_run: If True, don't actually delete files

        Returns:
            Tuple of (deleted_paths, errors)
        """
        all_files = self.get_all_files()
        deleted = []
        errors = []

        for entry in all_files:
            file_path = self.vendors_dir / entry.path

            try:
                if file_path.exists():
                    if not dry_run:
                        file_path.unlink()
                    deleted.append(file_path)
                else:
                    errors.append(f"File not found: {file_path}")

            except Exception as e:
                errors.append(f"Failed to delete {file_path}: {e}")

        if not dry_run and deleted:
            # Clear manifest
            self._manifest = self._create_empty_manifest()
            self.save_manifest()

        return deleted, errors

    @staticmethod
    def _calculate_checksum(file_path: Path) -> str:
        """Calculate SHA256 checksum of a file"""
        sha256 = hashlib.sha256()

        try:
            with open(file_path, 'rb') as f:
                while chunk := f.read(8192):
                    sha256.update(chunk)
            return sha256.hexdigest()
        except Exception as e:
            print(f"Warning: Failed to calculate checksum for {file_path}: {e}")
            return ""

    @staticmethod
    def _create_empty_manifest() -> Dict:
        """Create an empty manifest structure"""
        return {
            'version': ManifestManager.MANIFEST_VERSION,
            'last_updated': datetime.now().isoformat(),
            'files': {},
            'generators': {}
        }


# Global manifest manager instance
_global_manifest_manager: Optional[ManifestManager] = None


def get_global_manifest_manager() -> Optional[ManifestManager]:
    """Get the global manifest manager instance"""
    return _global_manifest_manager


def set_global_manifest_manager(manager: ManifestManager):
    """Set the global manifest manager instance"""
    global _global_manifest_manager
    _global_manifest_manager = manager
