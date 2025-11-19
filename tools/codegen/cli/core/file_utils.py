"""
File Utilities

Common file I/O utilities for code generation.

Owned by: library-quality-improvements spec
"""

import os
import shutil
from pathlib import Path
from typing import List, Optional
import hashlib


def ensure_directory(path: Path) -> Path:
    """
    Ensure directory exists, create if it doesn't.

    Args:
        path: Directory path

    Returns:
        Path to directory
    """
    path.mkdir(parents=True, exist_ok=True)
    return path


def clean_directory(path: Path, pattern: str = "*") -> int:
    """
    Remove all files matching pattern in directory.

    Args:
        path: Directory path
        pattern: Glob pattern for files to remove

    Returns:
        Number of files removed
    """
    if not path.exists():
        return 0

    count = 0
    for file_path in path.glob(pattern):
        if file_path.is_file():
            file_path.unlink()
            count += 1

    return count


def copy_file(src: Path, dst: Path, create_dirs: bool = True) -> Path:
    """
    Copy file from source to destination.

    Args:
        src: Source file path
        dst: Destination file path
        create_dirs: Create destination directory if it doesn't exist

    Returns:
        Destination path

    Raises:
        FileNotFoundError: If source doesn't exist
    """
    if not src.exists():
        raise FileNotFoundError(f"Source file not found: {src}")

    if create_dirs:
        ensure_directory(dst.parent)

    shutil.copy2(src, dst)
    return dst


def find_files(
    directory: Path,
    pattern: str = "*.cpp",
    recursive: bool = True
) -> List[Path]:
    """
    Find all files matching pattern in directory.

    Args:
        directory: Directory to search
        pattern: Glob pattern
        recursive: Search recursively

    Returns:
        List of matching file paths
    """
    if not directory.exists():
        return []

    if recursive:
        return list(directory.rglob(pattern))
    else:
        return list(directory.glob(pattern))


def read_text_file(path: Path, encoding: str = 'utf-8') -> str:
    """
    Read text file content.

    Args:
        path: File path
        encoding: Text encoding

    Returns:
        File content as string

    Raises:
        FileNotFoundError: If file doesn't exist
    """
    if not path.exists():
        raise FileNotFoundError(f"File not found: {path}")

    return path.read_text(encoding=encoding)


def write_text_file(
    path: Path,
    content: str,
    encoding: str = 'utf-8',
    create_dirs: bool = True
) -> Path:
    """
    Write text content to file.

    Args:
        path: File path
        content: Text content to write
        encoding: Text encoding
        create_dirs: Create parent directories if they don't exist

    Returns:
        File path
    """
    if create_dirs:
        ensure_directory(path.parent)

    path.write_text(content, encoding=encoding)
    return path


def file_hash(path: Path, algorithm: str = 'sha256') -> str:
    """
    Calculate hash of file content.

    Args:
        path: File path
        algorithm: Hash algorithm (md5, sha1, sha256)

    Returns:
        Hex digest of file hash

    Raises:
        FileNotFoundError: If file doesn't exist
    """
    if not path.exists():
        raise FileNotFoundError(f"File not found: {path}")

    hash_func = hashlib.new(algorithm)

    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            hash_func.update(chunk)

    return hash_func.hexdigest()


def files_identical(file1: Path, file2: Path) -> bool:
    """
    Check if two files have identical content.

    Args:
        file1: First file path
        file2: Second file path

    Returns:
        True if files are identical
    """
    if not file1.exists() or not file2.exists():
        return False

    return file_hash(file1) == file_hash(file2)


def get_relative_path(path: Path, base: Path) -> Path:
    """
    Get relative path from base to path.

    Args:
        path: Target path
        base: Base path

    Returns:
        Relative path
    """
    try:
        return path.relative_to(base)
    except ValueError:
        # Paths are not relative, return absolute
        return path


def safe_file_name(name: str) -> str:
    """
    Convert string to safe file name.

    Args:
        name: String to convert

    Returns:
        Safe file name
    """
    # Replace unsafe characters
    safe = name.replace('/', '_')
    safe = safe.replace('\\', '_')
    safe = safe.replace(':', '_')
    safe = safe.replace('*', '_')
    safe = safe.replace('?', '_')
    safe = safe.replace('"', '_')
    safe = safe.replace('<', '_')
    safe = safe.replace('>', '_')
    safe = safe.replace('|', '_')

    return safe


# Example usage
if __name__ == "__main__":
    # Example: ensure output directory exists
    output_dir = Path("build/generated")
    ensure_directory(output_dir)
    print(f"Created directory: {output_dir}")

    # Example: write generated code
    code = """
    #include <cstdint>

    uint32_t test() {
        return 42;
    }
    """

    output_file = output_dir / "test.cpp"
    write_text_file(output_file, code)
    print(f"Wrote file: {output_file}")

    # Example: calculate hash
    if output_file.exists():
        hash_value = file_hash(output_file)
        print(f"File hash: {hash_value}")
