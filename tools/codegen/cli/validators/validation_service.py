"""
Validation service - orchestrates all validators.

Provides high-level validation API for CLI commands.
"""

from pathlib import Path
from typing import List, Optional, Dict, Any
from dataclasses import dataclass

from .base import ValidationStage, ValidationResult
from .syntax_validator import SyntaxValidator
from .compile_validator import CompileValidator
from .semantic_validator import SemanticValidator
from .test_validator import TestValidator


@dataclass
class ValidationSummary:
    """Summary of validation run."""
    total_files: int = 0
    passed_files: int = 0
    failed_files: int = 0
    total_errors: int = 0
    total_warnings: int = 0
    total_duration_ms: float = 0.0
    results_by_stage: Dict[ValidationStage, List[ValidationResult]] = None

    def __post_init__(self):
        if self.results_by_stage is None:
            self.results_by_stage = {}

    @property
    def success_rate(self) -> float:
        """Calculate success rate percentage."""
        if self.total_files == 0:
            return 0.0
        return (self.passed_files / self.total_files) * 100


class ValidationService:
    """
    Validation service for generated code.

    Orchestrates multiple validation stages:
    - Syntax validation (Clang)
    - Semantic validation (SVD cross-reference) - coming soon
    - Compilation validation (ARM GCC) - coming soon
    - Test generation (Catch2) - coming soon
    """

    def __init__(self, config: Optional[Dict[str, Any]] = None):
        """
        Initialize validation service.

        Args:
            config: Configuration dict with validator settings
        """
        self.config = config or {}

        # Initialize validators
        self.syntax_validator = SyntaxValidator(
            clang_path=self.config.get("clang_path", "clang++")
        )
        self.compile_validator = CompileValidator(
            gcc_path=self.config.get("gcc_arm_path", "arm-none-eabi-gcc")
        )
        self.semantic_validator = SemanticValidator(
            svd_path=self.config.get("svd_path")
        )
        self.test_validator = TestValidator(
            catch2_path=self.config.get("catch2_path")
        )

    def validate_file(
        self,
        file_path: Path,
        stages: Optional[List[ValidationStage]] = None,
        **options
    ) -> List[ValidationResult]:
        """
        Validate a single file through specified stages.

        Args:
            file_path: Path to file to validate
            stages: List of stages to run (default: all available)
            **options: Validation options

        Returns:
            List of ValidationResult (one per stage)
        """
        if stages is None:
            stages = [ValidationStage.SYNTAX]  # Only syntax for now

        results = []

        for stage in stages:
            if stage == ValidationStage.SYNTAX:
                if self.syntax_validator.is_available():
                    result = self.syntax_validator.validate(
                        file_path,
                        include_paths=options.get("include_paths", []),
                        std=options.get("std", "c++23")
                    )
                    results.append(result)
                else:
                    # Create error result for unavailable validator
                    result = ValidationResult(stage=stage)
                    result.add_error(
                        "Syntax validator not available (clang++ not found)",
                        suggestion="Install clang or configure ALLOY_CLANG_PATH"
                    )
                    results.append(result)

            elif stage == ValidationStage.SEMANTIC:
                if self.semantic_validator.is_available():
                    result = self.semantic_validator.validate(
                        file_path,
                        svd_path=options.get("svd_path")
                    )
                    results.append(result)
                else:
                    # Create error result for unavailable validator
                    result = ValidationResult(stage=stage)
                    result.add_error(
                        "Semantic validator not available (no SVD file loaded)",
                        suggestion="Provide SVD file with --svd option or configure ALLOY_SVD_PATH"
                    )
                    results.append(result)

            elif stage == ValidationStage.COMPILE:
                if self.compile_validator.is_available():
                    result = self.compile_validator.validate(
                        file_path,
                        mcu=options.get("mcu", "cortex-m4"),
                        include_paths=options.get("include_paths", []),
                        defines=options.get("defines", {})
                    )
                    results.append(result)
                else:
                    # Create error result for unavailable validator
                    result = ValidationResult(stage=stage)
                    result.add_error(
                        "Compile validator not available (arm-none-eabi-gcc not found)",
                        suggestion="Install arm-none-eabi-gcc or set ALLOY_GCC_ARM_PATH"
                    )
                    results.append(result)

            elif stage == ValidationStage.TEST:
                if self.test_validator.is_available():
                    result = self.test_validator.validate(
                        file_path,
                        output_dir=options.get("test_output_dir"),
                        peripheral_name=options.get("peripheral_name"),
                        generate_only=options.get("generate_only", True)
                    )
                    results.append(result)
                else:
                    # Test generation is always available, but include this for consistency
                    result = ValidationResult(stage=stage)
                    result.add_error("Test validator not available")
                    results.append(result)

        return results

    def validate_directory(
        self,
        directory: Path,
        pattern: str = "**/*.hpp",
        stages: Optional[List[ValidationStage]] = None,
        **options
    ) -> ValidationSummary:
        """
        Validate all files in directory.

        Args:
            directory: Directory to scan
            pattern: Glob pattern for files
            stages: Validation stages to run
            **options: Validation options

        Returns:
            ValidationSummary with aggregate results
        """
        summary = ValidationSummary()

        # Find all files
        files = list(directory.glob(pattern))
        summary.total_files = len(files)

        # Validate each file
        for file_path in files:
            if not file_path.is_file():
                continue

            file_results = self.validate_file(file_path, stages=stages, **options)

            # Aggregate results
            file_passed = all(r.passed for r in file_results)
            if file_passed:
                summary.passed_files += 1
            else:
                summary.failed_files += 1

            # Count errors and warnings
            for result in file_results:
                summary.total_errors += result.error_count()
                summary.total_warnings += result.warning_count()
                summary.total_duration_ms += result.duration_ms

                # Group by stage
                if result.stage not in summary.results_by_stage:
                    summary.results_by_stage[result.stage] = []
                summary.results_by_stage[result.stage].append(result)

        return summary

    def get_available_stages(self) -> List[ValidationStage]:
        """
        Get list of available validation stages.

        Returns:
            List of stages that can run (validators are available)
        """
        available = []

        if self.syntax_validator.is_available():
            available.append(ValidationStage.SYNTAX)

        if self.semantic_validator.is_available():
            available.append(ValidationStage.SEMANTIC)

        if self.compile_validator.is_available():
            available.append(ValidationStage.COMPILE)

        if self.test_validator.is_available():
            available.append(ValidationStage.TEST)

        return available

    def check_requirements(self) -> Dict[str, bool]:
        """
        Check if all validation requirements are met.

        Returns:
            Dict mapping requirement name to availability status
        """
        return {
            "clang++": self.syntax_validator.is_available(),
            "svd_parser": self.semantic_validator.is_available(),
            "arm-none-eabi-gcc": self.compile_validator.is_available(),
            "test_generator": self.test_validator.is_available(),
        }
