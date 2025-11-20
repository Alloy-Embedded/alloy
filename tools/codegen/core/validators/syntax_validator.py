"""
Syntax Validator - C++ Syntax Validation using Clang

Validates C++ code syntax using Clang compiler frontend.
Checks for:
- Compilation errors
- Warnings (treated as errors with -Werror)
- Modern C++ compliance (C++20/23)
- Code style issues

Reference: See docs/cpp_code_generation_reference.md for code standards
"""

import subprocess
import tempfile
from pathlib import Path
from typing import List, Optional, Dict, Any
from dataclasses import dataclass
from enum import Enum


class ValidationLevel(Enum):
    """Validation strictness level"""
    STRICT = "strict"      # -Wall -Wextra -Werror -pedantic
    NORMAL = "normal"      # -Wall -Wextra
    PERMISSIVE = "permissive"  # Minimal checks


@dataclass
class ValidationResult:
    """Result of syntax validation"""
    valid: bool
    errors: List[str]
    warnings: List[str]
    file_path: Optional[Path] = None
    compiler_output: str = ""

    def __bool__(self) -> bool:
        return self.valid

    def format_report(self) -> str:
        """Format validation result as human-readable report"""
        if self.valid:
            return f"✅ Validation passed: {self.file_path or 'code snippet'}"

        report = [f"❌ Validation failed: {self.file_path or 'code snippet'}"]

        if self.errors:
            report.append(f"\nErrors ({len(self.errors)}):")
            for error in self.errors:
                report.append(f"  • {error}")

        if self.warnings:
            report.append(f"\nWarnings ({len(self.warnings)}):")
            for warning in self.warnings:
                report.append(f"  • {warning}")

        return "\n".join(report)


class SyntaxValidator:
    """
    Validates C++ code syntax using Clang.

    Features:
    - Validates against C++20/23 standards
    - Checks for common errors and warnings
    - Supports ARM Cortex-M target validation
    - Configurable strictness levels

    Example:
        validator = SyntaxValidator()

        # Validate a file
        result = validator.validate_file(Path("src/gpio.hpp"))
        if not result:
            print(result.format_report())

        # Validate code snippet
        code = '''
        #include <cstdint>
        constexpr uint32_t GPIO_BASE = 0x40020000UL;
        '''
        result = validator.validate_code(code)
    """

    def __init__(
        self,
        level: ValidationLevel = ValidationLevel.STRICT,
        cpp_standard: str = "c++23",
        target: str = "arm-none-eabi"
    ):
        """
        Initialize syntax validator.

        Args:
            level: Validation strictness level
            cpp_standard: C++ standard (c++20, c++23)
            target: Target architecture (arm-none-eabi, x86_64, etc.)
        """
        self.level = level
        self.cpp_standard = cpp_standard
        self.target = target
        self._clang_path = self._find_clang()

    def _find_clang(self) -> Optional[Path]:
        """Find Clang compiler in system PATH"""
        # Try clang++, clang, and versioned variants
        clang_names = ["clang++", "clang++-18", "clang++-17", "clang++-16", "clang"]

        for name in clang_names:
            try:
                result = subprocess.run(
                    ["which", name],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0 and result.stdout.strip():
                    return Path(result.stdout.strip())
            except (subprocess.TimeoutExpired, FileNotFoundError):
                continue

        return None

    def _get_clang_flags(self) -> List[str]:
        """Get Clang compiler flags based on validation level"""
        # Base flags
        flags = [
            f"-std={self.cpp_standard}",
            "-fsyntax-only",  # Only check syntax, don't compile
            "-fno-exceptions",  # Embedded systems don't use exceptions
            "-fno-rtti",  # No RTTI in embedded
        ]

        # Strictness level flags
        if self.level == ValidationLevel.STRICT:
            flags.extend([
                "-Wall",
                "-Wextra",
                "-Werror",  # Treat warnings as errors
                "-pedantic",
                "-Wshadow",
                "-Wconversion",
                "-Wsign-conversion",
                "-Wnon-virtual-dtor",
                "-Wold-style-cast",
                "-Wcast-align",
                "-Wunused",
                "-Woverloaded-virtual",
                "-Wpedantic",
            ])
        elif self.level == ValidationLevel.NORMAL:
            flags.extend(["-Wall", "-Wextra"])
        # PERMISSIVE: minimal flags

        # Target-specific flags
        if "arm" in self.target:
            flags.extend([
                "-target", "arm-none-eabi",
                "-mcpu=cortex-m4",
                "-mthumb",
                "-mfloat-abi=hard",
                "-mfpu=fpv4-sp-d16",
            ])

        return flags

    def validate_file(self, file_path: Path) -> ValidationResult:
        """
        Validate a C++ file.

        Args:
            file_path: Path to C++ file (.cpp, .hpp, .h)

        Returns:
            ValidationResult with errors and warnings
        """
        if not file_path.exists():
            return ValidationResult(
                valid=False,
                errors=[f"File not found: {file_path}"],
                warnings=[],
                file_path=file_path
            )

        if not self._clang_path:
            return ValidationResult(
                valid=False,
                errors=["Clang compiler not found in PATH"],
                warnings=["Install clang or clang++ to enable syntax validation"],
                file_path=file_path
            )

        # Run Clang syntax check
        cmd = [str(self._clang_path)] + self._get_clang_flags() + [str(file_path)]

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )

            return self._parse_clang_output(
                result.stdout + result.stderr,
                result.returncode == 0,
                file_path
            )

        except subprocess.TimeoutExpired:
            return ValidationResult(
                valid=False,
                errors=["Compilation timeout (>30s)"],
                warnings=[],
                file_path=file_path
            )
        except Exception as e:
            return ValidationResult(
                valid=False,
                errors=[f"Validation error: {str(e)}"],
                warnings=[],
                file_path=file_path
            )

    def validate_code(self, code: str, filename: str = "code.cpp") -> ValidationResult:
        """
        Validate a C++ code snippet.

        Args:
            code: C++ code as string
            filename: Virtual filename for error messages

        Returns:
            ValidationResult with errors and warnings
        """
        if not self._clang_path:
            return ValidationResult(
                valid=False,
                errors=["Clang compiler not found in PATH"],
                warnings=["Install clang or clang++ to enable syntax validation"]
            )

        # Create temporary file
        with tempfile.NamedTemporaryFile(
            mode='w',
            suffix='.cpp',
            delete=False
        ) as tmp:
            tmp.write(code)
            tmp_path = Path(tmp.name)

        try:
            result = self.validate_file(tmp_path)
            # Update file path to virtual filename
            result.file_path = Path(filename)
            return result
        finally:
            # Clean up temp file
            tmp_path.unlink(missing_ok=True)

    def validate_directory(self, directory: Path, pattern: str = "*.hpp") -> Dict[Path, ValidationResult]:
        """
        Validate all files matching pattern in directory.

        Args:
            directory: Directory to search
            pattern: Glob pattern for files (default: *.hpp)

        Returns:
            Dictionary mapping file paths to validation results
        """
        results = {}

        for file_path in directory.rglob(pattern):
            if file_path.is_file():
                results[file_path] = self.validate_file(file_path)

        return results

    def _parse_clang_output(
        self,
        output: str,
        success: bool,
        file_path: Optional[Path] = None
    ) -> ValidationResult:
        """Parse Clang compiler output into structured result"""
        errors = []
        warnings = []

        for line in output.splitlines():
            line = line.strip()
            if not line:
                continue

            # Parse error/warning lines
            if ": error:" in line:
                # Extract error message
                parts = line.split(": error:", 1)
                if len(parts) == 2:
                    errors.append(parts[1].strip())
                else:
                    errors.append(line)
            elif ": warning:" in line:
                # Extract warning message
                parts = line.split(": warning:", 1)
                if len(parts) == 2:
                    warnings.append(parts[1].strip())
                else:
                    warnings.append(line)

        return ValidationResult(
            valid=success and len(errors) == 0,
            errors=errors,
            warnings=warnings,
            file_path=file_path,
            compiler_output=output
        )

    def check_compliance(self, file_path: Path) -> Dict[str, Any]:
        """
        Check compliance with C++ code generation reference.

        Checks for:
        - constexpr usage
        - [[nodiscard]] attributes
        - static_assert validation
        - No macros (except include guards)
        - Modern C++ features

        Returns:
            Dictionary with compliance metrics
        """
        if not file_path.exists():
            return {"error": f"File not found: {file_path}"}

        code = file_path.read_text()

        # Count compliance indicators
        compliance = {
            "constexpr_count": code.count("constexpr"),
            "nodiscard_count": code.count("[[nodiscard]]"),
            "static_assert_count": code.count("static_assert"),
            "concept_count": code.count("concept"),
            "requires_count": code.count("requires"),
            "has_macros": "#define" in code and not code.count("#define") == code.count("#ifndef") + code.count("#define"),
            "has_exceptions": "throw" in code or "try" in code or "catch" in code,
            "has_rtti": "dynamic_cast" in code or "typeid" in code,
        }

        # Compliance score
        score = 0
        max_score = 0

        # Points for modern features
        if compliance["constexpr_count"] > 0:
            score += 2
        max_score += 2

        if compliance["nodiscard_count"] > 0:
            score += 1
        max_score += 1

        if compliance["static_assert_count"] > 0:
            score += 2
        max_score += 2

        # Penalties
        if compliance["has_macros"]:
            score -= 2

        if compliance["has_exceptions"]:
            score -= 3

        if compliance["has_rtti"]:
            score -= 2

        compliance["score"] = max(0, score)
        compliance["max_score"] = max_score
        compliance["percentage"] = (compliance["score"] / max_score * 100) if max_score > 0 else 0

        return compliance


# Example usage
if __name__ == "__main__":
    # Test syntax validator
    validator = SyntaxValidator(level=ValidationLevel.STRICT)

    # Example 1: Validate simple code
    code = """
    #include <cstdint>

    namespace gpio {
        constexpr uint32_t GPIOA_BASE = 0x40020000UL;

        [[nodiscard]] constexpr bool is_valid_pin(uint8_t pin) noexcept {
            return pin < 16;
        }
    }
    """

    result = validator.validate_code(code, "example.hpp")
    print(result.format_report())

    # Example 2: Check compliance
    # (Would need actual file to test)
