"""
Code Formatter Post-Processor

Automatically formats generated C++ code using clang-format to ensure
consistency with project code style.
"""

import subprocess
import shutil
from pathlib import Path
from typing import Optional
import logging

logger = logging.getLogger(__name__)


class CodeFormatter:
    """
    Post-processor that formats generated C++ code using clang-format.

    This ensures all generated code matches the project's code style defined
    in .clang-format, preventing CI/CD failures due to formatting issues.
    """

    def __init__(self, clang_format_binary: str = "clang-format"):
        """
        Initialize code formatter.

        Args:
            clang_format_binary: Path to clang-format executable (default: "clang-format")
        """
        self.clang_format_binary = clang_format_binary
        self._check_availability()

    def _check_availability(self) -> bool:
        """
        Check if clang-format is available.

        Returns:
            True if clang-format is found, False otherwise
        """
        if not shutil.which(self.clang_format_binary):
            logger.warning(
                f"clang-format not found at '{self.clang_format_binary}'. "
                "Generated code will not be formatted automatically."
            )
            return False
        return True

    def format_file(self, filepath: Path, dry_run: bool = False) -> bool:
        """
        Format a C++ file using clang-format.

        Args:
            filepath: Path to file to format
            dry_run: If True, check format but don't modify file

        Returns:
            True if formatting succeeded (or was not needed), False on error
        """
        if not filepath.exists():
            logger.error(f"File not found: {filepath}")
            return False

        # Only format C++ files
        if filepath.suffix not in {'.hpp', '.cpp', '.h', '.cc', '.cxx'}:
            return True

        if not shutil.which(self.clang_format_binary):
            # clang-format not available, skip formatting
            return True

        try:
            if dry_run:
                # Check if file needs formatting
                result = subprocess.run(
                    [self.clang_format_binary, '--dry-run', '--Werror', str(filepath)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                if result.returncode != 0:
                    logger.warning(f"File needs formatting: {filepath}")
                    return False
                return True
            else:
                # Format file in-place
                result = subprocess.run(
                    [self.clang_format_binary, '-i', str(filepath)],
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                if result.returncode != 0:
                    logger.error(f"Failed to format {filepath}: {result.stderr}")
                    return False

                logger.debug(f"Formatted: {filepath}")
                return True

        except subprocess.TimeoutExpired:
            logger.error(f"Timeout formatting {filepath}")
            return False
        except Exception as e:
            logger.error(f"Error formatting {filepath}: {e}")
            return False

    def format_string(self, code: str, filename: str = "temp.cpp") -> Optional[str]:
        """
        Format C++ code string using clang-format.

        Args:
            code: C++ code to format
            filename: Assumed filename for format detection (default: temp.cpp)

        Returns:
            Formatted code string, or None on error
        """
        if not shutil.which(self.clang_format_binary):
            # clang-format not available, return unformatted
            return code

        try:
            result = subprocess.run(
                [self.clang_format_binary, f'--assume-filename={filename}'],
                input=code,
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode != 0:
                logger.error(f"Failed to format code: {result.stderr}")
                return None

            return result.stdout

        except subprocess.TimeoutExpired:
            logger.error("Timeout formatting code string")
            return None
        except Exception as e:
            logger.error(f"Error formatting code string: {e}")
            return None

    def format_directory(
        self,
        directory: Path,
        pattern: str = "**/*.hpp",
        dry_run: bool = False
    ) -> tuple[int, int]:
        """
        Format all files matching pattern in directory.

        Args:
            directory: Directory to search
            pattern: Glob pattern for files to format (default: **/*.hpp)
            dry_run: If True, check format but don't modify files

        Returns:
            Tuple of (successful_count, failed_count)
        """
        if not directory.exists():
            logger.error(f"Directory not found: {directory}")
            return 0, 0

        files = list(directory.glob(pattern))
        success = 0
        failed = 0

        for file in files:
            if self.format_file(file, dry_run):
                success += 1
            else:
                failed += 1

        return success, failed


def format_generated_code(filepath: Path, verbose: bool = False) -> bool:
    """
    Convenience function to format a generated file.

    Args:
        filepath: Path to generated file
        verbose: Enable verbose logging

    Returns:
        True if formatting succeeded, False otherwise
    """
    if verbose:
        logging.basicConfig(level=logging.DEBUG)

    formatter = CodeFormatter()
    return formatter.format_file(filepath)


if __name__ == "__main__":
    # CLI for manual formatting
    import argparse

    parser = argparse.ArgumentParser(
        description="Format generated C++ code using clang-format"
    )
    parser.add_argument(
        'path',
        type=Path,
        help='File or directory to format'
    )
    parser.add_argument(
        '--pattern',
        default='**/*.hpp',
        help='Glob pattern for directory mode (default: **/*.hpp)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Check format without modifying files'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Verbose output'
    )

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG, format='%(message)s')
    else:
        logging.basicConfig(level=logging.INFO, format='%(message)s')

    formatter = CodeFormatter()

    if args.path.is_file():
        success = formatter.format_file(args.path, args.dry_run)
        exit(0 if success else 1)
    elif args.path.is_dir():
        success, failed = formatter.format_directory(
            args.path,
            args.pattern,
            args.dry_run
        )
        logger.info(f"Formatted {success} files, {failed} failed")
        exit(0 if failed == 0 else 1)
    else:
        logger.error(f"Path not found: {args.path}")
        exit(1)
