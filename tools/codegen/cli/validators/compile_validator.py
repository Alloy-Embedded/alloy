"""
Compile validator using ARM GCC.

Validates that generated code compiles successfully for target MCU.
"""

import subprocess
import shutil
import re
from pathlib import Path
from typing import Optional, List, Dict, Any
import time
import tempfile

from .base import Validator, ValidationResult, ValidationStage


class CompileValidator(Validator):
    """Validates C++ code compilation with ARM GCC."""

    def __init__(self, gcc_path: str = "arm-none-eabi-gcc"):
        super().__init__()
        self.stage = ValidationStage.COMPILE
        self.gcc_path = gcc_path

    def is_available(self) -> bool:
        """Check if ARM GCC is available."""
        return shutil.which(self.gcc_path) is not None

    def validate(
        self,
        file_path: Path,
        mcu: str = "cortex-m4",
        include_paths: Optional[List[Path]] = None,
        defines: Optional[Dict[str, str]] = None,
        **options
    ) -> ValidationResult:
        """
        Validate C++ file compilation with ARM GCC.

        Args:
            file_path: Path to C++ file
            mcu: MCU target (cortex-m0, cortex-m3, cortex-m4, cortex-m7, etc.)
            include_paths: Additional include directories
            defines: Preprocessor defines
            **options: Additional GCC options

        Returns:
            ValidationResult
        """
        result = ValidationResult(stage=self.stage)
        start_time = time.time()

        # Check file exists
        if not file_path.exists():
            result.add_error(f"File not found: {file_path}")
            return result

        # Check GCC is available
        if not self.is_available():
            result.add_error(
                f"ARM GCC not found: {self.gcc_path}",
                suggestion="Install arm-none-eabi-gcc or set ALLOY_GCC_ARM_PATH"
            )
            return result

        # Create temporary output file
        with tempfile.NamedTemporaryFile(suffix=".o", delete=False) as tmp:
            output_file = Path(tmp.name)

        try:
            # Build GCC command
            cmd = self._build_gcc_command(
                file_path,
                output_file,
                mcu,
                include_paths,
                defines,
                options
            )

            # Run GCC
            try:
                proc = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=60
                )

                # Parse output
                self._parse_gcc_output(proc.stderr, file_path, result)

                # Check if compilation succeeded
                if proc.returncode == 0 and not result.has_errors():
                    result.add_info("✓ Compilation successful")

                    # Get object file size
                    if output_file.exists():
                        size = output_file.stat().st_size
                        result.metadata["object_size_bytes"] = size
                        result.add_info(f"Object file size: {size} bytes")

                    result.metadata["compiler"] = self.gcc_path
                    result.metadata["mcu"] = mcu

            except subprocess.TimeoutExpired:
                result.add_error(
                    "Compilation timed out (60s)",
                    suggestion="Code may have infinite template instantiation"
                )
            except Exception as e:
                result.add_error(f"Compilation failed: {e}")

        finally:
            # Clean up temporary file
            if output_file.exists():
                output_file.unlink()

        # Record duration
        result.duration_ms = (time.time() - start_time) * 1000

        return result

    def _build_gcc_command(
        self,
        file_path: Path,
        output_file: Path,
        mcu: str,
        include_paths: Optional[List[Path]],
        defines: Optional[Dict[str, str]],
        options: Dict[str, Any]
    ) -> List[str]:
        """Build ARM GCC command."""
        cmd = [
            self.gcc_path,
            "-c",  # Compile only, don't link
            f"-mcpu={mcu}",
            "-mthumb",  # Thumb instruction set
            "-std=c++23",
            "-O0",  # No optimization for validation
            "-Wall",
            "-Wextra",
            "-pedantic",
            "-fno-exceptions",  # Embedded: no exceptions
            "-fno-rtti",  # Embedded: no RTTI
        ]

        # Add include paths
        if include_paths:
            for path in include_paths:
                cmd.extend(["-I", str(path)])

        # Add defines
        if defines:
            for key, value in defines.items():
                if value:
                    cmd.append(f"-D{key}={value}")
                else:
                    cmd.append(f"-D{key}")

        # Add output file
        cmd.extend(["-o", str(output_file)])

        # Add input file
        cmd.append(str(file_path))

        return cmd

    def _parse_gcc_output(
        self,
        output: str,
        file_path: Path,
        result: ValidationResult
    ):
        """
        Parse GCC error/warning output.

        Format:
        file.cpp:10:5: error: expected ';' before 'return'
        """
        if not output:
            return

        # Regex for GCC messages
        # Example: gpio.hpp:42:18: error: expected ';' after struct definition
        pattern = r"^(.+?):(\d+):(\d+):\s+(error|warning|note):\s+(.+)$"

        lines = output.strip().split('\n')
        for line in lines:
            match = re.match(pattern, line)

            if match:
                file = match.group(1)
                line_num = int(match.group(2))
                col_num = int(match.group(3))
                severity = match.group(4)
                message = match.group(5)

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

    def get_compiler_info(self) -> Optional[Dict[str, str]]:
        """
        Get ARM GCC compiler information.

        Returns:
            Dict with version, target, etc. or None if not available
        """
        if not self.is_available():
            return None

        try:
            proc = subprocess.run(
                [self.gcc_path, "--version"],
                capture_output=True,
                text=True,
                timeout=5
            )

            if proc.returncode == 0:
                lines = proc.stdout.strip().split('\n')
                version_line = lines[0] if lines else ""

                return {
                    "path": self.gcc_path,
                    "version": version_line,
                    "available": True
                }

        except Exception:
            pass

        return None

    def validate_with_test_program(
        self,
        file_path: Path,
        mcu: str = "cortex-m4",
        **options
    ) -> ValidationResult:
        """
        Validate by compiling with a minimal test program.

        Creates a temporary main() that includes the file.
        """
        result = ValidationResult(stage=self.stage)

        # Create temporary test program
        with tempfile.NamedTemporaryFile(
            mode='w',
            suffix='.cpp',
            delete=False
        ) as tmp:
            tmp.write(f"""
// Auto-generated test program for validation
#include "{file_path.absolute()}"

int main() {{
    // Minimal test - just check compilation
    return 0;
}}
""")
            test_file = Path(tmp.name)

        try:
            # Validate test program
            test_result = self.validate(test_file, mcu=mcu, **options)

            # Copy results but change file path to original
            result.passed = test_result.passed
            result.metadata = test_result.metadata
            result.duration_ms = test_result.duration_ms

            for msg in test_result.messages:
                if msg.file_path == test_file:
                    msg.file_path = file_path
                result.messages.append(msg)

        finally:
            # Clean up test file
            if test_file.exists():
                test_file.unlink()

        return result
