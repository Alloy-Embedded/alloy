"""
Compile Validator

Full ARM GCC compilation test for generated code.

Owned by: library-quality-improvements spec
Consumed by: CLI ValidationService wrapper
"""

import subprocess
import tempfile
from pathlib import Path
from typing import List, Optional
from dataclasses import dataclass

from .syntax_validator import ValidationLevel, ValidationResult


class CompileValidator:
    """
    Compiles generated code for target MCU using ARM GCC.

    Validates that generated code actually compiles for the target architecture.
    """

    def __init__(self, gcc_path: str = "arm-none-eabi-gcc"):
        """
        Initialize compile validator.

        Args:
            gcc_path: Path to ARM GCC executable
        """
        self.gcc_path = gcc_path

    def validate(
        self,
        source_files: List[Path],
        target: str,
        include_dirs: Optional[List[Path]] = None,
        defines: Optional[List[str]] = None
    ) -> ValidationResult:
        """
        Compile generated code for target MCU.

        Args:
            source_files: List of C++ source files to compile
            target: Target MCU (e.g., "STM32F401xE")
            include_dirs: Additional include directories
            defines: Preprocessor defines

        Returns:
            ValidationResult with compilation results
        """
        if not source_files:
            return ValidationResult(
                passed=False,
                level=ValidationLevel.ERROR,
                message="No source files provided"
            )

        # Check all source files exist
        for src in source_files:
            if not src.exists():
                return ValidationResult(
                    passed=False,
                    level=ValidationLevel.ERROR,
                    message=f"Source file not found: {src}",
                    file_path=src
                )

        # Build compiler command
        cmd = [
            self.gcc_path,
            '-mcpu=cortex-m4',  # Default, should be configurable per target
            '-mthumb',
            '-mfloat-abi=hard',
            '-mfpu=fpv4-sp-d16',
            '-std=c++23',
            '-c',  # Compile only, don't link
            '-Wall',
            '-Wextra',
            '-Werror',  # Treat warnings as errors
        ]

        # Add include directories
        if include_dirs:
            for inc_dir in include_dirs:
                cmd.extend(['-I', str(inc_dir)])

        # Add defines
        if defines:
            for define in defines:
                cmd.append(f'-D{define}')

        # Add source files
        cmd.extend([str(src) for src in source_files])

        # Create temp output directory
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_out = Path(temp_dir) / "output.o"
            cmd.extend(['-o', str(temp_out)])

            try:
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=30
                )

                if result.returncode == 0:
                    return ValidationResult(
                        passed=True,
                        level=ValidationLevel.INFO,
                        message=f"Compilation successful for {target}"
                    )
                else:
                    return ValidationResult(
                        passed=False,
                        level=ValidationLevel.ERROR,
                        message=f"Compilation failed:\n{result.stderr}"
                    )

            except subprocess.TimeoutExpired:
                return ValidationResult(
                    passed=False,
                    level=ValidationLevel.ERROR,
                    message="Compilation timed out (>30s)"
                )

            except FileNotFoundError:
                return ValidationResult(
                    passed=False,
                    level=ValidationLevel.ERROR,
                    message=f"ARM GCC not found at: {self.gcc_path}"
                )

    def validate_single(
        self,
        source_file: Path,
        target: str,
        include_dirs: Optional[List[Path]] = None
    ) -> ValidationResult:
        """
        Compile a single source file.

        Args:
            source_file: C++ source file
            target: Target MCU
            include_dirs: Include directories

        Returns:
            ValidationResult
        """
        return self.validate([source_file], target, include_dirs)


# Example usage
if __name__ == "__main__":
    validator = CompileValidator()

    # Example: compile GPIO for STM32F4
    gpio_src = Path("src/hal/vendors/st/stm32f4/gpio.cpp")
    include_dirs = [
        Path("src/hal/api"),
        Path("src/core"),
    ]

    if gpio_src.exists():
        result = validator.validate_single(
            gpio_src,
            target="STM32F401xE",
            include_dirs=include_dirs
        )
        print(f"Compile validation: {result.passed} - {result.message}")
