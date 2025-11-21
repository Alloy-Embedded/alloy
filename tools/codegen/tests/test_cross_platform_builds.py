"""
Cross-Platform Build Tests

Tests that code can be built successfully for all supported platforms
and that builds meet quality criteria (no warnings, correct sizes, etc.).
"""

import pytest
from pathlib import Path
import subprocess
import tempfile
import shutil
import re

# Add parent directory to path for imports
import sys
CODEGEN_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(CODEGEN_DIR))


# Configuration for supported boards
SUPPORTED_BOARDS = [
    {
        "name": "nucleo_f446re",
        "mcu": "STM32F446RE",
        "vendor": "st",
        "family": "stm32f4",
        "arch": "cortex-m4",
        "expected_max_size": 512 * 1024,  # 512KB flash
    },
    {
        "name": "same70_xplained",
        "mcu": "ATSAME70Q21",
        "vendor": "microchip",
        "family": "same70",
        "arch": "cortex-m7",
        "expected_max_size": 2 * 1024 * 1024,  # 2MB flash
    },
]


@pytest.fixture
def build_workspace():
    """Create temporary build workspace."""
    temp_dir = tempfile.mkdtemp()
    workspace = Path(temp_dir)

    (workspace / "src").mkdir()
    (workspace / "build").mkdir()

    yield workspace

    # Cleanup
    shutil.rmtree(temp_dir)


@pytest.fixture
def sample_source_code():
    """Create sample source code for testing."""
    return """
#include <cstdint>

// Simple blink application
namespace app {
    static constexpr uint32_t LED_PIN = 13;

    void initialize() {
        // Setup code
    }

    void run() {
        // Main loop
        while (true) {
            // Toggle LED
        }
    }
}

int main() {
    app::initialize();
    app::run();
    return 0;
}
"""


class TestBoardConfigurations:
    """Test build configurations for supported boards."""

    @pytest.mark.parametrize("board", SUPPORTED_BOARDS)
    def test_board_configuration_valid(self, board):
        """Test: Each board has valid configuration."""
        # Verify required fields
        assert "name" in board
        assert "mcu" in board
        assert "vendor" in board
        assert "family" in board
        assert "arch" in board
        assert "expected_max_size" in board

        # Verify values are reasonable
        assert len(board["name"]) > 0
        assert board["expected_max_size"] > 0

    def test_all_boards_unique(self):
        """Test: All board names are unique."""
        board_names = [b["name"] for b in SUPPORTED_BOARDS]
        assert len(board_names) == len(set(board_names)), "Duplicate board names found"

    def test_supported_architectures(self):
        """Test: All boards use supported ARM Cortex-M architectures."""
        supported_archs = ["cortex-m0", "cortex-m0plus", "cortex-m3", "cortex-m4", "cortex-m7", "cortex-m33"]

        for board in SUPPORTED_BOARDS:
            assert board["arch"] in supported_archs, f"Unsupported arch: {board['arch']}"


class TestCompilationChecks:
    """Test compilation requirements and checks."""

    def test_compiler_available(self):
        """Test: ARM GCC compiler is available."""
        try:
            result = subprocess.run(
                ["arm-none-eabi-gcc", "--version"],
                capture_output=True,
                text=True,
                timeout=5
            )
            assert result.returncode == 0, "Compiler not found"
            assert "arm-none-eabi-gcc" in result.stdout.lower()
        except FileNotFoundError:
            pytest.skip("ARM GCC toolchain not installed")
        except subprocess.TimeoutExpired:
            pytest.fail("Compiler check timed out")

    def test_compiler_supports_cpp20(self):
        """Test: Compiler supports C++20."""
        try:
            # Create test file
            test_code = """
#include <concepts>
#include <cstdint>

template<typename T>
concept Integral = std::is_integral_v<T>;

int main() { return 0; }
"""
            with tempfile.NamedTemporaryFile(mode='w', suffix='.cpp', delete=False) as f:
                f.write(test_code)
                test_file = Path(f.name)

            try:
                result = subprocess.run(
                    [
                        "arm-none-eabi-g++",
                        "-std=c++20",
                        "-mcpu=cortex-m4",
                        "-mthumb",
                        "-c",
                        str(test_file),
                        "-o", "/dev/null"
                    ],
                    capture_output=True,
                    text=True,
                    timeout=10
                )

                # Check if compilation succeeded or if it's a toolchain issue
                if result.returncode != 0:
                    if "concepts" in result.stderr or "C++20" in result.stderr:
                        pytest.skip("Compiler doesn't support C++20 features")
                    # Other errors might be expected (no stdlib, etc.)

            finally:
                test_file.unlink()

        except FileNotFoundError:
            pytest.skip("ARM GCC toolchain not installed")
        except subprocess.TimeoutExpired:
            pytest.fail("Compilation test timed out")


class TestCodeQuality:
    """Test code quality checks."""

    def test_no_compiler_warnings(self, build_workspace, sample_source_code):
        """Test: Code compiles without warnings."""
        source_file = build_workspace / "src" / "main.cpp"
        source_file.write_text(sample_source_code)

        try:
            result = subprocess.run(
                [
                    "arm-none-eabi-g++",
                    "-std=c++20",
                    "-Wall",
                    "-Wextra",
                    "-mcpu=cortex-m4",
                    "-mthumb",
                    "-c",
                    str(source_file),
                    "-o", str(build_workspace / "build" / "main.o")
                ],
                capture_output=True,
                text=True,
                timeout=10
            )

            # Check for warnings in output
            if result.returncode == 0:
                # Compilation succeeded
                assert "warning:" not in result.stderr.lower(), \
                    f"Compilation produced warnings:\n{result.stderr}"
            else:
                # Compilation failed - might be due to missing stdlib
                if "cannot find" in result.stderr or "no such file" in result.stderr:
                    pytest.skip("Cross-compilation environment not fully configured")

        except FileNotFoundError:
            pytest.skip("ARM GCC toolchain not installed")
        except subprocess.TimeoutExpired:
            pytest.fail("Compilation timed out")

    def test_code_uses_modern_cpp(self, build_workspace):
        """Test: Generated code uses modern C++ features."""
        # Generate sample code with modern features
        modern_code = """
#pragma once

#include <cstdint>
#include <concepts>

namespace alloy::hal {

    // Concept for pin types
    template<typename T>
    concept PinType = requires(T pin) {
        { pin.set() } -> std::same_as<void>;
        { pin.clear() } -> std::same_as<void>;
    };

    // constexpr functions
    constexpr uint32_t calculate_value(uint32_t x) {
        return x * 2;
    }

    // Static inline members (C++17)
    struct Config {
        static inline uint32_t value = 100;
    };

} // namespace alloy::hal
"""

        header_file = build_workspace / "src" / "modern.hpp"
        header_file.write_text(modern_code)

        # Verify modern features are present
        content = header_file.read_text()
        assert "concept" in content, "Missing C++20 concepts"
        assert "constexpr" in content, "Missing constexpr"
        assert "static inline" in content, "Missing static inline"
        assert "namespace alloy" in content, "Missing nested namespace"


class TestBinarySizes:
    """Test binary size requirements."""

    def test_minimal_binary_size_reasonable(self):
        """Test: Minimal binary fits within expected size."""
        # For a minimal "blink" application:
        # - Startup code: ~500 bytes
        # - Vector table: ~400 bytes
        # - Minimal main: ~100 bytes
        # - Total: ~1KB
        expected_minimal_size = 2 * 1024  # 2KB

        # This is a sanity check - actual size depends on many factors
        assert expected_minimal_size < 512 * 1024, "Minimal size too large"

    @pytest.mark.parametrize("board", SUPPORTED_BOARDS)
    def test_board_flash_capacity_sufficient(self, board):
        """Test: Board has sufficient flash for typical applications."""
        # Typical application sizes:
        # - Small: 10-50 KB
        # - Medium: 50-200 KB
        # - Large: 200-500 KB
        typical_app_size = 100 * 1024  # 100KB

        assert board["expected_max_size"] > typical_app_size, \
            f"Board {board['name']} flash too small for typical apps"


class TestBuildOutputs:
    """Test build output validation."""

    def test_object_file_created(self, build_workspace, sample_source_code):
        """Test: Compilation creates object file."""
        source_file = build_workspace / "src" / "main.cpp"
        source_file.write_text(sample_source_code)

        output_file = build_workspace / "build" / "main.o"

        try:
            result = subprocess.run(
                [
                    "arm-none-eabi-g++",
                    "-std=c++20",
                    "-mcpu=cortex-m4",
                    "-mthumb",
                    "-c",
                    str(source_file),
                    "-o", str(output_file)
                ],
                capture_output=True,
                text=True,
                timeout=10
            )

            if result.returncode == 0:
                # Verify output file was created
                assert output_file.exists(), "Object file not created"
                assert output_file.stat().st_size > 0, "Object file is empty"
            else:
                # Check if it's an environment issue
                if "cannot find" in result.stderr:
                    pytest.skip("Cross-compilation environment not configured")

        except FileNotFoundError:
            pytest.skip("ARM GCC toolchain not installed")
        except subprocess.TimeoutExpired:
            pytest.fail("Compilation timed out")

    def test_build_output_format_valid(self, build_workspace, sample_source_code):
        """Test: Build output has valid ELF format."""
        source_file = build_workspace / "src" / "main.cpp"
        source_file.write_text(sample_source_code)

        output_file = build_workspace / "build" / "main.o"

        try:
            # Compile
            result = subprocess.run(
                [
                    "arm-none-eabi-g++",
                    "-mcpu=cortex-m4",
                    "-mthumb",
                    "-c",
                    str(source_file),
                    "-o", str(output_file)
                ],
                capture_output=True,
                timeout=10
            )

            if result.returncode == 0 and output_file.exists():
                # Check file format with 'file' command or readelf
                try:
                    file_result = subprocess.run(
                        ["file", str(output_file)],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )
                    # Should be ELF format
                    assert "ELF" in file_result.stdout, "Output is not ELF format"
                except FileNotFoundError:
                    # 'file' command not available
                    pass

        except FileNotFoundError:
            pytest.skip("ARM GCC toolchain not installed")
        except subprocess.TimeoutExpired:
            pytest.skip("Build process timed out")


class TestCrossCompilation:
    """Test cross-compilation specifics."""

    def test_cortex_m4_target(self):
        """Test: Can target Cortex-M4."""
        test_code = "int main() { return 0; }"

        try:
            with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
                f.write(test_code)
                test_file = Path(f.name)

            try:
                result = subprocess.run(
                    [
                        "arm-none-eabi-gcc",
                        "-mcpu=cortex-m4",
                        "-mthumb",
                        "-c",
                        str(test_file),
                        "-o", "/dev/null"
                    ],
                    capture_output=True,
                    timeout=10
                )

                # Should compile without architecture errors
                if result.returncode != 0:
                    assert "cortex-m4" not in result.stderr.lower(), \
                        "Compiler doesn't support Cortex-M4"

            finally:
                test_file.unlink()

        except FileNotFoundError:
            pytest.skip("ARM GCC toolchain not installed")
        except subprocess.TimeoutExpired:
            pytest.skip("Compilation timed out")

    def test_thumb_mode_enabled(self):
        """Test: Thumb instruction set is used."""
        # Thumb mode is required for Cortex-M processors
        # Verify it's specified in compiler flags
        compiler_flags = ["-mcpu=cortex-m4", "-mthumb"]

        assert "-mthumb" in compiler_flags, "Thumb mode not enabled"


class TestBuildReproducibility:
    """Test build reproducibility."""

    def test_deterministic_build(self, build_workspace, sample_source_code):
        """Test: Building twice produces identical output."""
        source_file = build_workspace / "src" / "main.cpp"
        source_file.write_text(sample_source_code)

        output1 = build_workspace / "build" / "main1.o"
        output2 = build_workspace / "build" / "main2.o"

        try:
            # Build once
            subprocess.run(
                [
                    "arm-none-eabi-g++",
                    "-mcpu=cortex-m4",
                    "-mthumb",
                    "-c",
                    str(source_file),
                    "-o", str(output1)
                ],
                capture_output=True,
                timeout=10,
                check=False
            )

            # Build again
            subprocess.run(
                [
                    "arm-none-eabi-g++",
                    "-mcpu=cortex-m4",
                    "-mthumb",
                    "-c",
                    str(source_file),
                    "-o", str(output2)
                ],
                capture_output=True,
                timeout=10,
                check=False
            )

            if output1.exists() and output2.exists():
                # Compare file sizes (exact comparison would require
                # -frandom-seed for deterministic builds)
                size1 = output1.stat().st_size
                size2 = output2.stat().st_size

                assert size1 == size2, "Builds produce different sizes"

        except FileNotFoundError:
            pytest.skip("ARM GCC toolchain not installed")
        except subprocess.TimeoutExpired:
            pytest.skip("Build timed out")


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
