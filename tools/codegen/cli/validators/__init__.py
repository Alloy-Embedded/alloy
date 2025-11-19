"""
Code validation framework for Alloy CLI.

Provides multi-stage validation of generated code:
- Syntax validation (Clang)
- Semantic validation (SVD cross-reference)
- Compilation validation (ARM GCC)
- Test generation (Catch2)
"""

from .base import ValidationStage, ValidationResult, Validator
from .syntax_validator import SyntaxValidator
from .compile_validator import CompileValidator
from .test_validator import TestValidator
from .test_generator import TestGenerator, TestCategory, TestCase, TestSuite
from .validation_service import ValidationService

__all__ = [
    "ValidationStage",
    "ValidationResult",
    "Validator",
    "SyntaxValidator",
    "CompileValidator",
    "TestValidator",
    "TestGenerator",
    "TestCategory",
    "TestCase",
    "TestSuite",
    "ValidationService",
]
