"""
Test validator - generates and runs tests for generated code.

Uses TestGenerator to create Catch2 tests and validates code correctness.
"""

import subprocess
import shutil
from pathlib import Path
from typing import Optional, List
import time
import tempfile

from .base import Validator, ValidationResult, ValidationStage
from .test_generator import TestGenerator


class TestValidator(Validator):
    """Validates generated code by creating and running tests."""

    def __init__(self, catch2_path: Optional[str] = None):
        super().__init__()
        self.stage = ValidationStage.TEST
        self.catch2_path = catch2_path
        self.generator = TestGenerator()

    def is_available(self) -> bool:
        """
        Check if test generation is available.

        Returns:
            True (test generation always available)
        """
        # Test generation doesn't require external tools
        # (running tests requires compiler but generation doesn't)
        return True

    def validate(
        self,
        file_path: Path,
        output_dir: Optional[Path] = None,
        peripheral_name: Optional[str] = None,
        generate_only: bool = False,
        **options
    ) -> ValidationResult:
        """
        Validate C++ file by generating tests.

        Args:
            file_path: Path to C++ header file
            output_dir: Directory for generated tests (temp if None)
            peripheral_name: Name of peripheral (auto-detected if None)
            generate_only: Only generate tests, don't run them
            **options: Additional options

        Returns:
            ValidationResult
        """
        result = ValidationResult(stage=self.stage)
        start_time = time.time()

        # Check file exists
        if not file_path.exists():
            result.add_error(f"File not found: {file_path}")
            return result

        # Create output directory
        if output_dir is None:
            temp_output = True
            output_dir = Path(tempfile.mkdtemp(prefix="alloy_tests_"))
        else:
            temp_output = False
            output_dir.mkdir(parents=True, exist_ok=True)

        try:
            # Generate tests
            test_file = self.generator.generate_from_header(
                file_path,
                output_dir,
                peripheral_name
            )

            result.add_info(f"Generated test file: {test_file.name}")
            result.metadata["test_file"] = str(test_file)

            # Parse header to get test statistics
            definitions = self.generator.parse_header(file_path)
            result.metadata["peripherals_found"] = len(definitions.get('peripherals', {}))
            result.metadata["registers_found"] = len(definitions.get('registers', {}))
            result.metadata["bitfields_found"] = len(definitions.get('bitfields', {}))

            # Generate summary
            suite = self.generator.generate_peripheral_tests(
                definitions,
                peripheral_name or file_path.stem.upper()
            )
            summary = self.generator.get_test_summary(suite)
            result.metadata["tests_generated"] = summary['total']

            result.add_info(
                f"Generated {summary['total']} tests "
                f"({summary['base_address']} base address, "
                f"{summary['register_offset']} register, "
                f"{summary['bitfield']} bitfield)"
            )

            if not generate_only:
                # Try to compile and run tests (if compiler available)
                # This is optional - just generating tests is valid
                result.add_info("Test generation completed (compilation not implemented yet)")

        except Exception as e:
            result.add_error(f"Test generation failed: {e}")

        finally:
            # Clean up temp directory if we created it
            if temp_output:
                import shutil
                shutil.rmtree(output_dir, ignore_errors=True)

        # Record duration
        result.duration_ms = (time.time() - start_time) * 1000

        return result

    def validate_directory(
        self,
        directory: Path,
        output_dir: Path,
        pattern: str = "*.hpp",
        **options
    ) -> List[ValidationResult]:
        """
        Generate tests for all headers in directory.

        Args:
            directory: Directory to scan
            output_dir: Directory for generated tests
            pattern: Glob pattern for files
            **options: Validation options

        Returns:
            List of ValidationResult for each file
        """
        results = []

        for file_path in directory.glob(pattern):
            if file_path.is_file():
                result = self.validate(
                    file_path,
                    output_dir=output_dir,
                    **options
                )
                results.append(result)

        return results

    def get_generator(self) -> TestGenerator:
        """
        Get the TestGenerator instance.

        Returns:
            TestGenerator
        """
        return self.generator
