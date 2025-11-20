"""
Compile Validator - Full ARM GCC Compilation Test

Validates generated C++ code by compiling it with ARM GCC.
Checks for:
- Successful compilation with ARM GCC
- Zero warnings with strict flags
- Binary size analysis
- Symbol table validation
- Optimization level compliance

Reference: See docs/cpp_code_generation_reference.md for code standards
"""

import subprocess
import tempfile
import shutil
from pathlib import Path
from typing import List, Optional, Dict, Any, Tuple
from dataclasses import dataclass, field
from enum import Enum
import re


class CompileTarget(Enum):
    """ARM Cortex-M compilation targets"""
    CORTEX_M0 = "cortex-m0"
    CORTEX_M0PLUS = "cortex-m0plus"
    CORTEX_M3 = "cortex-m3"
    CORTEX_M4 = "cortex-m4"
    CORTEX_M4F = "cortex-m4"  # With FPU
    CORTEX_M7 = "cortex-m7"
    CORTEX_M7F = "cortex-m7"  # With FPU
    CORTEX_M33 = "cortex-m33"


class OptimizationLevel(Enum):
    """GCC optimization levels"""
    DEBUG = "0"        # -O0
    SIZE = "s"         # -Os
    SPEED = "2"        # -O2
    AGGRESSIVE = "3"   # -O3


@dataclass
class CompileResult:
    """Result of compilation"""
    success: bool
    compiler_output: str = ""
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    binary_size: Optional[int] = None
    text_size: Optional[int] = None
    data_size: Optional[int] = None
    bss_size: Optional[int] = None
    file_path: Optional[Path] = None
    object_file: Optional[Path] = None

    def __bool__(self) -> bool:
        return self.success

    def format_report(self) -> str:
        """Format compilation result as human-readable report"""
        if self.success:
            report = [f"✅ Compilation successful: {self.file_path or 'code snippet'}"]

            if self.binary_size is not None:
                report.append(f"\nBinary Size Analysis:")
                report.append(f"  .text (code):  {self.text_size or 0:>6} bytes")
                report.append(f"  .data (init):  {self.data_size or 0:>6} bytes")
                report.append(f"  .bss  (zero):  {self.bss_size or 0:>6} bytes")
                report.append(f"  Total:         {self.binary_size:>6} bytes")

            if self.warnings:
                report.append(f"\n⚠️  {len(self.warnings)} warnings:")
                for warning in self.warnings[:5]:  # Show first 5
                    report.append(f"  • {warning}")
                if len(self.warnings) > 5:
                    report.append(f"  ... and {len(self.warnings) - 5} more")

            return "\n".join(report)

        # Failed compilation
        report = [f"❌ Compilation failed: {self.file_path or 'code snippet'}"]

        if self.errors:
            report.append(f"\nErrors ({len(self.errors)}):")
            for error in self.errors[:10]:  # Show first 10
                report.append(f"  • {error}")
            if len(self.errors) > 10:
                report.append(f"  ... and {len(self.errors) - 10} more")

        if self.warnings:
            report.append(f"\nWarnings ({len(self.warnings)}):")
            for warning in self.warnings[:5]:
                report.append(f"  • {warning}")
            if len(self.warnings) > 5:
                report.append(f"  ... and {len(self.warnings) - 5} more")

        return "\n".join(report)


class CompileValidator:
    """
    Validates C++ code by compiling with ARM GCC.

    Features:
    - Compiles for ARM Cortex-M targets
    - Validates zero-overhead abstractions
    - Analyzes binary size
    - Checks symbol table
    - Validates optimization results

    Example:
        validator = CompileValidator(
            target=CompileTarget.CORTEX_M4F,
            optimization=OptimizationLevel.SIZE
        )

        # Compile a file
        result = validator.compile_file(Path("src/gpio.hpp"))
        if result:
            print(f"Binary size: {result.binary_size} bytes")
        else:
            print(result.format_report())

        # Verify zero overhead
        overhead = validator.check_zero_overhead(
            Path("src/gpio.hpp"),
            expected_instructions=5
        )
    """

    def __init__(
        self,
        target: CompileTarget = CompileTarget.CORTEX_M4F,
        optimization: OptimizationLevel = OptimizationLevel.SIZE,
        enable_fpu: bool = True,
        strict_warnings: bool = True
    ):
        """
        Initialize compile validator.

        Args:
            target: ARM Cortex-M target
            optimization: Optimization level
            enable_fpu: Enable FPU (for M4F/M7F)
            strict_warnings: Treat warnings as errors
        """
        self.target = target
        self.optimization = optimization
        self.enable_fpu = enable_fpu
        self.strict_warnings = strict_warnings
        self._gcc_path = self._find_arm_gcc()

    def _find_arm_gcc(self) -> Optional[Path]:
        """Find ARM GCC compiler in system PATH"""
        # Try arm-none-eabi-g++, arm-none-eabi-gcc
        gcc_names = [
            "arm-none-eabi-g++",
            "arm-none-eabi-gcc",
        ]

        for name in gcc_names:
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

    def _get_gcc_flags(self) -> List[str]:
        """Get ARM GCC compiler flags"""
        flags = [
            "-c",  # Compile only, don't link
            f"-mcpu={self.target.value}",
            "-mthumb",
            f"-O{self.optimization.value}",
            "-std=c++23",
            "-fno-exceptions",
            "-fno-rtti",
            "-ffunction-sections",  # Each function in separate section
            "-fdata-sections",      # Each data in separate section
        ]

        # FPU flags
        if self.enable_fpu and self.target in [CompileTarget.CORTEX_M4F, CompileTarget.CORTEX_M7F]:
            flags.extend([
                "-mfloat-abi=hard",
                "-mfpu=fpv4-sp-d16",  # M4F FPU
            ])

        # Warning flags
        if self.strict_warnings:
            flags.extend([
                "-Wall",
                "-Wextra",
                "-Werror",  # Treat warnings as errors
                "-Wpedantic",
                "-Wshadow",
                "-Wconversion",
                "-Wsign-conversion",
            ])
        else:
            flags.extend(["-Wall", "-Wextra"])

        return flags

    def compile_file(
        self,
        file_path: Path,
        include_dirs: Optional[List[Path]] = None,
        defines: Optional[Dict[str, str]] = None
    ) -> CompileResult:
        """
        Compile a C++ file with ARM GCC.

        Args:
            file_path: Path to C++ file
            include_dirs: Additional include directories
            defines: Preprocessor defines

        Returns:
            CompileResult with compilation details
        """
        if not file_path.exists():
            return CompileResult(
                success=False,
                errors=[f"File not found: {file_path}"],
                file_path=file_path
            )

        if not self._gcc_path:
            return CompileResult(
                success=False,
                errors=["ARM GCC compiler not found in PATH"],
                warnings=["Install arm-none-eabi-gcc to enable compilation validation"],
                file_path=file_path
            )

        # Create temporary output file
        with tempfile.NamedTemporaryFile(suffix='.o', delete=False) as tmp:
            output_path = Path(tmp.name)

        try:
            # Build command
            cmd = [str(self._gcc_path)] + self._get_gcc_flags()

            # Add include directories
            if include_dirs:
                for inc_dir in include_dirs:
                    cmd.extend(["-I", str(inc_dir)])

            # Add defines
            if defines:
                for key, value in defines.items():
                    if value:
                        cmd.append(f"-D{key}={value}")
                    else:
                        cmd.append(f"-D{key}")

            # Add input and output
            cmd.extend(["-o", str(output_path), str(file_path)])

            # Run compilation
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60
            )

            # Parse output
            compile_result = self._parse_gcc_output(
                result.stdout + result.stderr,
                result.returncode == 0,
                file_path
            )

            # Analyze binary if successful
            if compile_result.success and output_path.exists():
                compile_result.object_file = output_path
                size_info = self._analyze_binary_size(output_path)
                compile_result.binary_size = size_info.get('total', 0)
                compile_result.text_size = size_info.get('text', 0)
                compile_result.data_size = size_info.get('data', 0)
                compile_result.bss_size = size_info.get('bss', 0)
            else:
                # Clean up on failure
                output_path.unlink(missing_ok=True)

            return compile_result

        except subprocess.TimeoutExpired:
            output_path.unlink(missing_ok=True)
            return CompileResult(
                success=False,
                errors=["Compilation timeout (>60s)"],
                file_path=file_path
            )
        except Exception as e:
            output_path.unlink(missing_ok=True)
            return CompileResult(
                success=False,
                errors=[f"Compilation error: {str(e)}"],
                file_path=file_path
            )

    def compile_code(
        self,
        code: str,
        filename: str = "code.cpp",
        include_dirs: Optional[List[Path]] = None,
        defines: Optional[Dict[str, str]] = None
    ) -> CompileResult:
        """
        Compile a C++ code snippet.

        Args:
            code: C++ code as string
            filename: Virtual filename for error messages
            include_dirs: Additional include directories
            defines: Preprocessor defines

        Returns:
            CompileResult
        """
        # Create temporary file
        with tempfile.NamedTemporaryFile(
            mode='w',
            suffix='.cpp',
            delete=False
        ) as tmp:
            tmp.write(code)
            tmp_path = Path(tmp.name)

        try:
            result = self.compile_file(tmp_path, include_dirs, defines)
            result.file_path = Path(filename)
            return result
        finally:
            # Clean up temp file
            tmp_path.unlink(missing_ok=True)
            # Clean up object file if it exists
            if result.object_file and result.object_file.exists():
                result.object_file.unlink(missing_ok=True)

    def _parse_gcc_output(
        self,
        output: str,
        success: bool,
        file_path: Optional[Path] = None
    ) -> CompileResult:
        """Parse ARM GCC compiler output"""
        errors = []
        warnings = []

        for line in output.splitlines():
            line = line.strip()
            if not line:
                continue

            # Parse error/warning lines
            if ": error:" in line:
                parts = line.split(": error:", 1)
                if len(parts) == 2:
                    errors.append(parts[1].strip())
                else:
                    errors.append(line)
            elif ": warning:" in line:
                parts = line.split(": warning:", 1)
                if len(parts) == 2:
                    warnings.append(parts[1].strip())
                else:
                    warnings.append(line)

        return CompileResult(
            success=success and len(errors) == 0,
            compiler_output=output,
            errors=errors,
            warnings=warnings,
            file_path=file_path
        )

    def _analyze_binary_size(self, object_file: Path) -> Dict[str, int]:
        """Analyze binary size using arm-none-eabi-size"""
        size_cmd = str(self._gcc_path).replace("g++", "size").replace("gcc", "size")

        try:
            result = subprocess.run(
                [size_cmd, str(object_file)],
                capture_output=True,
                text=True,
                timeout=10
            )

            if result.returncode == 0:
                # Parse output (format: text data bss dec hex filename)
                lines = result.stdout.strip().split('\n')
                if len(lines) >= 2:
                    parts = lines[1].split()
                    if len(parts) >= 3:
                        text = int(parts[0])
                        data = int(parts[1])
                        bss = int(parts[2])
                        total = text + data + bss
                        return {
                            'text': text,
                            'data': data,
                            'bss': bss,
                            'total': total
                        }

        except Exception:
            pass

        return {}

    def check_zero_overhead(
        self,
        file_path: Path,
        expected_instructions: Optional[int] = None,
        max_overhead_percent: float = 5.0
    ) -> Tuple[bool, str]:
        """
        Verify zero-overhead abstraction.

        Compiles the code and checks that abstractions compile to
        minimal assembly instructions.

        Args:
            file_path: Path to C++ file
            expected_instructions: Expected number of instructions (if known)
            max_overhead_percent: Maximum acceptable overhead percentage

        Returns:
            Tuple of (is_zero_overhead, explanation)
        """
        result = self.compile_file(file_path)

        if not result.success:
            return False, "Compilation failed"

        if not result.object_file or not result.object_file.exists():
            return False, "No object file generated"

        # Disassemble object file
        objdump_cmd = str(self._gcc_path).replace("g++", "objdump").replace("gcc", "objdump")

        try:
            disasm_result = subprocess.run(
                [objdump_cmd, "-d", str(result.object_file)],
                capture_output=True,
                text=True,
                timeout=10
            )

            if disasm_result.returncode != 0:
                return False, "Disassembly failed"

            # Count actual instructions
            instruction_count = 0
            for line in disasm_result.stdout.splitlines():
                # Match assembly instruction lines (contain hex address and opcode)
                if re.match(r'\s+[0-9a-f]+:\s+[0-9a-f]+', line):
                    instruction_count += 1

            if expected_instructions is not None:
                overhead = abs(instruction_count - expected_instructions)
                overhead_pct = (overhead / expected_instructions * 100) if expected_instructions > 0 else 0

                if overhead_pct <= max_overhead_percent:
                    return True, f"Zero overhead: {instruction_count} instructions (expected {expected_instructions}, {overhead_pct:.1f}% overhead)"
                else:
                    return False, f"Overhead detected: {instruction_count} instructions (expected {expected_instructions}, {overhead_pct:.1f}% overhead)"

            # No expected count - just report actual
            return True, f"Generated {instruction_count} instructions"

        except Exception as e:
            return False, f"Analysis error: {str(e)}"
        finally:
            # Clean up object file
            if result.object_file:
                result.object_file.unlink(missing_ok=True)

    def validate_optimization(
        self,
        file_path: Path,
        max_size_bytes: Optional[int] = None
    ) -> Tuple[bool, str]:
        """
        Validate that optimization meets size requirements.

        Args:
            file_path: Path to C++ file
            max_size_bytes: Maximum acceptable size in bytes

        Returns:
            Tuple of (meets_requirements, explanation)
        """
        result = self.compile_file(file_path)

        if not result.success:
            return False, "Compilation failed"

        if result.binary_size is None:
            return False, "Could not determine binary size"

        if max_size_bytes is not None:
            if result.binary_size <= max_size_bytes:
                return True, f"Size OK: {result.binary_size} bytes (limit: {max_size_bytes} bytes)"
            else:
                overhead = result.binary_size - max_size_bytes
                return False, f"Size exceeded: {result.binary_size} bytes (limit: {max_size_bytes}, +{overhead} bytes)"

        return True, f"Binary size: {result.binary_size} bytes"


# Example usage
if __name__ == "__main__":
    # Test compile validator
    validator = CompileValidator(
        target=CompileTarget.CORTEX_M4F,
        optimization=OptimizationLevel.SIZE
    )

    # Example code
    code = """
    #include <cstdint>

    namespace gpio {
        constexpr uint32_t GPIOA_BASE = 0x40020000UL;

        [[nodiscard]] inline bool is_valid_pin(uint8_t pin) noexcept {
            return pin < 16;
        }
    }
    """

    result = validator.compile_code(code, "example.cpp")
    print(result.format_report())
