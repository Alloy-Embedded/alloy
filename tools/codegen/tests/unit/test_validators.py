"""
Unit tests for validation framework and validators.
"""

import pytest
from pathlib import Path
import tempfile

# Import validators
import sys
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.validators import (
    ValidationStage,
    ValidationResult,
    SyntaxValidator,
    CompileValidator,
    ValidationService
)


class TestValidationResult:
    """Test ValidationResult class."""

    def test_empty_result_passes(self):
        """Test that empty result passes."""
        result = ValidationResult(stage=ValidationStage.SYNTAX)
        assert result.passed
        assert not result.has_errors()
        assert result.error_count() == 0
        assert result.warning_count() == 0

    def test_add_error_marks_failed(self):
        """Test that adding error marks result as failed."""
        result = ValidationResult(stage=ValidationStage.SYNTAX)
        result.add_error("Test error")

        assert not result.passed
        assert result.has_errors()
        assert result.error_count() == 1

    def test_add_warning_does_not_fail(self):
        """Test that warnings don't fail validation."""
        result = ValidationResult(stage=ValidationStage.SYNTAX)
        result.add_warning("Test warning")

        assert result.passed
        assert not result.has_errors()
        assert result.warning_count() == 1

    def test_add_info(self):
        """Test adding info messages."""
        result = ValidationResult(stage=ValidationStage.SYNTAX)
        result.add_info("Test info")

        assert result.passed
        assert len(result.messages) == 1


class TestSyntaxValidator:
    """Test SyntaxValidator with Clang."""

    def test_is_available(self):
        """Test checking if clang is available."""
        validator = SyntaxValidator()
        # Just check that is_available() returns a boolean
        assert isinstance(validator.is_available(), bool)

    def test_validate_valid_cpp_file(self, valid_cpp_file):
        """Test validation of valid C++ file."""
        validator = SyntaxValidator()

        if not validator.is_available():
            pytest.skip("clang++ not available")

        result = validator.validate(valid_cpp_file)

        # Should have no errors
        assert not result.has_errors()
        assert result.stage == ValidationStage.SYNTAX

    def test_validate_invalid_cpp_file(self, invalid_cpp_file):
        """Test validation of invalid C++ file."""
        validator = SyntaxValidator()

        if not validator.is_available():
            pytest.skip("clang++ not available")

        result = validator.validate(invalid_cpp_file)

        # Should have errors
        assert result.has_errors()
        assert result.error_count() > 0

    def test_validate_nonexistent_file(self):
        """Test validation of non-existent file."""
        validator = SyntaxValidator()
        result = validator.validate(Path("/nonexistent/file.hpp"))

        assert result.has_errors()
        assert result.error_count() == 1


class TestCompileValidator:
    """Test CompileValidator with ARM GCC."""

    def test_is_available(self):
        """Test checking if ARM GCC is available."""
        validator = CompileValidator()
        # Just check that is_available() returns a boolean
        assert isinstance(validator.is_available(), bool)

    def test_validate_valid_cpp_file(self, valid_cpp_file):
        """Test compilation of valid C++ file."""
        validator = CompileValidator()

        if not validator.is_available():
            pytest.skip("arm-none-eabi-gcc not available")

        result = validator.validate(valid_cpp_file, mcu="cortex-m4")

        # Should compile successfully
        assert not result.has_errors()
        assert result.stage == ValidationStage.COMPILE
        assert "object_size_bytes" in result.metadata

    def test_validate_invalid_cpp_file(self, invalid_cpp_file):
        """Test compilation of invalid C++ file."""
        validator = CompileValidator()

        if not validator.is_available():
            pytest.skip("arm-none-eabi-gcc not available")

        result = validator.validate(invalid_cpp_file, mcu="cortex-m4")

        # Should have compilation errors
        assert result.has_errors()
        assert result.error_count() > 0

    def test_validate_nonexistent_file(self):
        """Test compilation of non-existent file."""
        validator = CompileValidator()
        result = validator.validate(Path("/nonexistent/file.hpp"))

        assert result.has_errors()
        assert result.error_count() == 1

    def test_validate_different_mcus(self, valid_cpp_file):
        """Test compilation for different MCU targets."""
        validator = CompileValidator()

        if not validator.is_available():
            pytest.skip("arm-none-eabi-gcc not available")

        mcus = ["cortex-m0", "cortex-m3", "cortex-m4", "cortex-m7"]

        for mcu in mcus:
            result = validator.validate(valid_cpp_file, mcu=mcu)
            assert not result.has_errors(), f"Failed for MCU: {mcu}"
            assert result.metadata.get("mcu") == mcu

    def test_get_compiler_info(self):
        """Test getting compiler information."""
        validator = CompileValidator()

        if not validator.is_available():
            pytest.skip("arm-none-eabi-gcc not available")

        info = validator.get_compiler_info()

        assert info is not None
        assert "path" in info
        assert "version" in info
        assert info["available"] is True


class TestValidationService:
    """Test ValidationService orchestration."""

    def test_create_service(self):
        """Test creating validation service."""
        service = ValidationService()
        assert service is not None

    def test_get_available_stages(self):
        """Test getting available validation stages."""
        service = ValidationService()
        stages = service.get_available_stages()

        # Should always return a list
        assert isinstance(stages, list)

        # If clang is available, SYNTAX should be in list
        if service.syntax_validator.is_available():
            assert ValidationStage.SYNTAX in stages

        # If ARM GCC is available, COMPILE should be in list
        if service.compile_validator.is_available():
            assert ValidationStage.COMPILE in stages

    def test_check_requirements(self):
        """Test checking validation requirements."""
        service = ValidationService()
        requirements = service.check_requirements()

        assert isinstance(requirements, dict)
        assert "clang++" in requirements
        assert "arm-none-eabi-gcc" in requirements
        assert "svd_files" in requirements

        # Values should be booleans
        for key, value in requirements.items():
            assert isinstance(value, bool)

    def test_validate_file_syntax_stage(self, valid_cpp_file):
        """Test validating file with syntax stage."""
        service = ValidationService()

        if not service.syntax_validator.is_available():
            pytest.skip("clang++ not available")

        results = service.validate_file(
            valid_cpp_file,
            stages=[ValidationStage.SYNTAX]
        )

        assert len(results) == 1
        assert results[0].stage == ValidationStage.SYNTAX
        assert not results[0].has_errors()

    def test_validate_file_compile_stage(self, valid_cpp_file):
        """Test validating file with compile stage."""
        service = ValidationService()

        if not service.compile_validator.is_available():
            pytest.skip("arm-none-eabi-gcc not available")

        results = service.validate_file(
            valid_cpp_file,
            stages=[ValidationStage.COMPILE],
            mcu="cortex-m4"
        )

        assert len(results) == 1
        assert results[0].stage == ValidationStage.COMPILE
        assert not results[0].has_errors()

    def test_validate_file_all_available_stages(self, valid_cpp_file):
        """Test validating file with all available stages."""
        service = ValidationService()

        # Get available stages
        available_stages = service.get_available_stages()

        if not available_stages:
            pytest.skip("No validators available")

        results = service.validate_file(
            valid_cpp_file,
            stages=available_stages
        )

        assert len(results) == len(available_stages)

        # All should pass for valid file
        for result in results:
            assert not result.has_errors()

    def test_validate_directory(self, temp_dir, valid_cpp_file):
        """Test validating directory."""
        service = ValidationService()

        if not service.syntax_validator.is_available():
            pytest.skip("clang++ not available")

        # Copy valid file to temp directory
        test_file = temp_dir / "test.hpp"
        test_file.write_text(valid_cpp_file.read_text())

        summary = service.validate_directory(
            temp_dir,
            pattern="*.hpp",
            stages=[ValidationStage.SYNTAX]
        )

        assert summary.total_files == 1
        assert summary.passed_files == 1
        assert summary.failed_files == 0
        assert summary.total_errors == 0


# Fixtures

@pytest.fixture
def valid_cpp_file(temp_dir):
    """Create a valid C++ file for testing."""
    file_path = temp_dir / "valid.hpp"
    file_path.write_text("""
#ifndef VALID_HPP
#define VALID_HPP

namespace test {

template<typename T>
class Container {
public:
    Container() = default;

    void add(T value) {
        // Implementation
    }

private:
    T data;
};

} // namespace test

#endif // VALID_HPP
""")
    return file_path


@pytest.fixture
def invalid_cpp_file(temp_dir):
    """Create an invalid C++ file for testing."""
    file_path = temp_dir / "invalid.hpp"
    file_path.write_text("""
#ifndef INVALID_HPP
#define INVALID_HPP

// Missing semicolon after class
class Broken {
public:
    void method()
}

// Syntax error: missing template argument
template<>
class Empty;

#endif // INVALID_HPP
""")
    return file_path


@pytest.fixture
def temp_dir():
    """Create temporary directory for tests."""
    with tempfile.TemporaryDirectory() as tmpdir:
        yield Path(tmpdir)
