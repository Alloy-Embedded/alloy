"""
Syntax validator using Clang.

Validates C++ syntax and basic semantic checks using clang++.
"""

import subprocess
import shutil
import re
from pathlib import Path
from typing import Optional, List
import time

from .base import Validator, ValidationResult, ValidationStage


class SyntaxValidator(Validator):
    """Validates C++ syntax using Clang."""

    def __init__(self, clang_path: str = "clang++"):
        super().__init__()
        self.stage = ValidationStage.SYNTAX
        self.clang_path = clang_path

    def is_available(self) -> bool:
        """Check if clang++ is available."""
        return shutil.which(self.clang_path) is not None

    def validate(
        self,
        file_path: Path,
        include_paths: Optional[List[Path]] = None,
        std: str = "c++23",
        **options
    ) -> ValidationResult:
        """
        Validate C++ file syntax with Clang.

        Args:
            file_path: Path to C++ file
            include_paths: Additional include directories
            std: C++ standard (c++11, c++14, c++17, c++20, c++23)
            **options: Additional clang options

        Returns:
            ValidationResult
        """
        result = ValidationResult(stage=self.stage)
        start_time = time.time()

        # Check file exists
        if not file_path.exists():
            result.add_error(f"File not found: {file_path}")
            return result

        # Check clang is available
        if not self.is_available():
            result.add_error(
                f"Clang not found: {self.clang_path}",
                suggestion="Install clang or set ALLOY_CLANG_PATH"
            )
            return result

        # Build clang command
        cmd = [
            self.clang_path,
            "-fsyntax-only",  # Only check syntax, don't generate code
            f"-std={std}",
            "-Wall",  # All warnings
            "-Wextra",  # Extra warnings
            "-pedantic",  # Strict standard compliance
        ]

        # Add include paths
        if include_paths:
            for path in include_paths:
                cmd.extend(["-I", str(path)])

        # Add file
        cmd.append(str(file_path))

        # Run clang
        try:
            proc = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )

            # Parse output
            self._parse_clang_output(proc.stderr, file_path, result)

            # If no errors found and clang succeeded
            if proc.returncode == 0 and not result.has_errors():
                result.add_info(f"✓ Syntax validation passed")
                result.metadata["compiler"] = self.clang_path
                result.metadata["standard"] = std

        except subprocess.TimeoutExpired:
            result.add_error(
                "Clang validation timed out (30s)",
                suggestion="File may have infinite template recursion"
            )
        except Exception as e:
            result.add_error(f"Clang validation failed: {e}")

        # Record duration
        result.duration_ms = (time.time() - start_time) * 1000

        return result

    def _parse_clang_output(
        self,
        output: str,
        file_path: Path,
        result: ValidationResult
    ):
        """
        Parse clang error/warning output.

        Format:
        file.cpp:10:5: error: expected ';' after expression
        """
        if not output:
            return

        # Regex to parse clang messages
        # Example: gpio.hpp:42:18: error: expected ';' after struct definition
        pattern = r"^(.+?):(\d+):(\d+):\s+(error|warning|note):\s+(.+)$"

        lines = output.strip().split('\n')
        i = 0
        while i < len(lines):
            line = lines[i]
            match = re.match(pattern, line)

            if match:
                file = match.group(1)
                line_num = int(match.group(2))
                col_num = int(match.group(3))
                severity = match.group(4)
                message = match.group(5)

                # Get code snippet if available (next line often has it)
                code_snippet = None
                if i + 1 < len(lines) and not re.match(pattern, lines[i + 1]):
                    code_snippet = lines[i + 1].strip()

                # Add message
                if severity == "error":
                    result.add_error(
                        message,
                        file_path=Path(file),
                        line=line_num
                    )
                elif severity == "warning":
                    result.add_warning(
                        message,
                        file_path=Path(file),
                        line=line_num
                    )
                else:  # note
                    result.add_info(message, file_path=Path(file))

            i += 1

    def validate_directory(
        self,
        directory: Path,
        pattern: str = "**/*.hpp",
        **options
    ) -> List[ValidationResult]:
        """
        Validate all C++ files in directory.

        Args:
            directory: Directory to scan
            pattern: Glob pattern for files
            **options: Validation options

        Returns:
            List of ValidationResult for each file
        """
        results = []
        for file in directory.glob(pattern):
            if file.is_file():
                result = self.validate(file, **options)
                results.append(result)

        return results
