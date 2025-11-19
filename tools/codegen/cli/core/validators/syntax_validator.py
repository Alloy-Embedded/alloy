"""
Syntax Validator

Validates generated C++ code syntax using Clang AST parser.

Owned by: library-quality-improvements spec
Consumed by: CLI ValidationService wrapper
"""

import subprocess
import tempfile
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional
from enum import Enum


class ValidationLevel(Enum):
    """Validation severity levels"""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class ValidationResult:
    """Result of a validation check"""
    passed: bool
    level: ValidationLevel
    message: str
    file_path: Optional[Path] = None
    line_number: Optional[int] = None
    column: Optional[int] = None


class SyntaxValidator:
    """
    Core syntax validator - checks C++ code compiles.

    Uses clang to perform syntax-only compilation checks.
    """

    def __init__(self, clang_path: str = "clang++"):
        """
        Initialize syntax validator.

        Args:
            clang_path: Path to clang++ executable
        """
        self.clang_path = clang_path

    def validate(self, code: str, std: str = "c++23") -> ValidationResult:
        """
        Run clang syntax check on C++ code.

        Args:
            code: C++ source code to validate
            std: C++ standard (c++17, c++20, c++23)

        Returns:
            ValidationResult with syntax check results
        """
        # Create temporary file for code
        with tempfile.NamedTemporaryFile(mode='w', suffix='.cpp', delete=False) as f:
            f.write(code)
            temp_path = Path(f.name)

        try:
            # Run clang syntax check
            result = subprocess.run(
                [
                    self.clang_path,
                    f'-std={std}',
                    '-fsyntax-only',
                    '-fno-color-diagnostics',
                    str(temp_path)
                ],
                capture_output=True,
                text=True,
                timeout=10
            )

            if result.returncode == 0:
                return ValidationResult(
                    passed=True,
                    level=ValidationLevel.INFO,
                    message="Syntax validation passed"
                )
            else:
                # Parse error output
                errors = self._parse_clang_errors(result.stderr)
                return ValidationResult(
                    passed=False,
                    level=ValidationLevel.ERROR,
                    message=f"Syntax errors found:\n{errors}"
                )

        except subprocess.TimeoutExpired:
            return ValidationResult(
                passed=False,
                level=ValidationLevel.ERROR,
                message="Syntax validation timed out (>10s)"
            )

        except FileNotFoundError:
            return ValidationResult(
                passed=False,
                level=ValidationLevel.ERROR,
                message=f"Clang not found at: {self.clang_path}"
            )

        finally:
            # Clean up temp file
            temp_path.unlink(missing_ok=True)

    def validate_file(self, file_path: Path, std: str = "c++23") -> ValidationResult:
        """
        Validate a C++ file.

        Args:
            file_path: Path to C++ file
            std: C++ standard

        Returns:
            ValidationResult
        """
        if not file_path.exists():
            return ValidationResult(
                passed=False,
                level=ValidationLevel.ERROR,
                message=f"File not found: {file_path}",
                file_path=file_path
            )

        code = file_path.read_text()
        result = self.validate(code, std)
        result.file_path = file_path
        return result

    def _parse_clang_errors(self, stderr: str) -> str:
        """
        Parse clang error output into readable format.

        Args:
            stderr: Clang stderr output

        Returns:
            Formatted error string
        """
        lines = stderr.split('\n')
        errors = []

        for line in lines:
            if 'error:' in line or 'warning:' in line:
                errors.append(line.strip())

        return '\n'.join(errors) if errors else stderr


# Example usage (for documentation)
if __name__ == "__main__":
    validator = SyntaxValidator()

    # Test valid code
    valid_code = """
    #include <cstdint>

    int main() {
        uint32_t x = 42;
        return 0;
    }
    """

    result = validator.validate(valid_code)
    print(f"Valid code: {result.passed} - {result.message}")

    # Test invalid code
    invalid_code = """
    int main() {
        invalid_syntax here;
        return 0;
    }
    """

    result = validator.validate(invalid_code)
    print(f"Invalid code: {result.passed} - {result.message}")
