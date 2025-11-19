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
    TestValidator,
    TestGenerator,
    TestCategory,
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


class TestTestGenerator:
    """Test TestGenerator for Catch2 test generation."""

    def test_create_generator(self):
        """Test creating test generator."""
        generator = TestGenerator()
        assert generator is not None

    def test_parse_header_with_peripherals(self, peripheral_header):
        """Test parsing header with peripheral definitions."""
        generator = TestGenerator()
        definitions = generator.parse_header(peripheral_header)

        assert 'peripherals' in definitions
        assert 'registers' in definitions
        assert 'bitfields' in definitions
        assert len(definitions['peripherals']) > 0

    def test_parse_header_extracts_base_addresses(self, peripheral_header):
        """Test extracting peripheral base addresses."""
        generator = TestGenerator()
        definitions = generator.parse_header(peripheral_header)

        # Should find GPIOA_BASE
        assert 'GPIOA' in definitions['peripherals']
        assert definitions['peripherals']['GPIOA'] == 0x40020000

    def test_parse_header_extracts_register_offsets(self, peripheral_header):
        """Test extracting register offsets."""
        generator = TestGenerator()
        definitions = generator.parse_header(peripheral_header)

        # Should find register offsets
        assert len(definitions['registers']) > 0

    def test_parse_header_extracts_bitfields(self, peripheral_header):
        """Test extracting bitfield positions."""
        generator = TestGenerator()
        definitions = generator.parse_header(peripheral_header)

        # Should find bitfield positions
        assert len(definitions['bitfields']) > 0

    def test_generate_peripheral_tests(self, peripheral_header):
        """Test generating test suite for peripheral."""
        generator = TestGenerator()
        definitions = generator.parse_header(peripheral_header)

        suite = generator.generate_peripheral_tests(definitions, "GPIO")

        assert suite.name == "GPIO_Tests"
        assert len(suite.test_cases) > 0

    def test_generate_test_file(self, temp_dir, peripheral_header):
        """Test generating complete test file."""
        generator = TestGenerator()
        definitions = generator.parse_header(peripheral_header)

        suite = generator.generate_peripheral_tests(definitions, "GPIO")

        output_file = temp_dir / "test_gpio.cpp"
        result = generator.generate_test_file(suite, output_file)

        assert result.exists()
        assert result.stat().st_size > 0

        # Check file contains Catch2 includes
        content = result.read_text()
        assert "#include <catch2/catch_test_macros.hpp>" in content
        assert "TEST_CASE" in content

    def test_generate_from_header(self, temp_dir, peripheral_header):
        """Test generating test from header file."""
        generator = TestGenerator()

        output_file = generator.generate_from_header(
            peripheral_header,
            temp_dir,
            "GPIO"
        )

        assert output_file.exists()
        assert output_file.name == "test_peripheral.hpp"

    def test_get_test_summary(self, peripheral_header):
        """Test getting test summary statistics."""
        generator = TestGenerator()
        definitions = generator.parse_header(peripheral_header)
        suite = generator.generate_peripheral_tests(definitions, "GPIO")

        summary = generator.get_test_summary(suite)

        assert 'total' in summary
        assert 'base_address' in summary
        assert 'register_offset' in summary
        assert 'bitfield' in summary
        assert summary['total'] == len(suite.test_cases)


class TestTestValidator:
    """Test TestValidator for test generation validation."""

    def test_is_available(self):
        """Test checking if test validator is available."""
        validator = TestValidator()
        # Test generation is always available
        assert validator.is_available() is True

    def test_validate_generates_tests(self, temp_dir, peripheral_header):
        """Test that validate generates test files."""
        validator = TestValidator()

        result = validator.validate(
            peripheral_header,
            output_dir=temp_dir,
            peripheral_name="GPIO",
            generate_only=True
        )

        # Should succeed
        assert not result.has_errors()
        assert result.stage == ValidationStage.TEST

        # Should report test file generation
        assert "test_file" in result.metadata
        assert result.metadata["tests_generated"] > 0

    def test_validate_reports_statistics(self, peripheral_header, temp_dir):
        """Test that validate reports generation statistics."""
        validator = TestValidator()

        result = validator.validate(
            peripheral_header,
            output_dir=temp_dir,
            generate_only=True
        )

        # Should report statistics
        assert "peripherals_found" in result.metadata
        assert "registers_found" in result.metadata
        assert "bitfields_found" in result.metadata
        assert "tests_generated" in result.metadata

    def test_get_generator(self):
        """Test getting TestGenerator instance."""
        validator = TestValidator()
        generator = validator.get_generator()

        assert generator is not None
        assert isinstance(generator, TestGenerator)


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
def peripheral_header(temp_dir):
    """Create a peripheral header file for testing."""
    file_path = temp_dir / "peripheral.hpp"
    file_path.write_text("""
#ifndef PERIPHERAL_HPP
#define PERIPHERAL_HPP

#include <cstdint>
#include <cstddef>

// Peripheral base addresses
namespace gpio {
    constexpr std::uintptr_t GPIOA_BASE = 0x40020000;
    constexpr std::uintptr_t GPIOB_BASE = 0x40020400;

    // Register offsets
    static constexpr std::size_t MODER_OFFSET = 0x00;
    static constexpr std::size_t ODR_OFFSET = 0x14;
    static constexpr std::size_t IDR_OFFSET = 0x10;

    // Bitfield positions
    static constexpr std::uint32_t PIN0_POS = 0;
    static constexpr std::uint32_t PIN1_POS = 1;
    static constexpr std::uint32_t PIN2_POS = 2;
}

#endif // PERIPHERAL_HPP
""")
    return file_path


@pytest.fixture
def temp_dir():
    """Create temporary directory for tests."""
    with tempfile.TemporaryDirectory() as tmpdir:
        yield Path(tmpdir)
