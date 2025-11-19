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
from .semantic_validator import SemanticValidator
from .test_validator import TestValidator
from .test_generator import TestGenerator, TestCategory, TestCase, TestSuite
from .svd_parser import SVDParser, Peripheral, Register, BitField, DeviceInfo
from .code_parser import CodeParser, CodePeripheral, CodeRegister, CodeBitField
from .validation_service import ValidationService

__all__ = [
    "ValidationStage",
    "ValidationResult",
    "Validator",
    "SyntaxValidator",
    "CompileValidator",
    "SemanticValidator",
    "TestValidator",
    "TestGenerator",
    "TestCategory",
    "TestCase",
    "TestSuite",
    "SVDParser",
    "Peripheral",
    "Register",
    "BitField",
    "DeviceInfo",
    "CodeParser",
    "CodePeripheral",
    "CodeRegister",
    "CodeBitField",
    "ValidationService",
]
