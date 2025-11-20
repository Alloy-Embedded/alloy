"""
Core Validators for Generated C++ Code

This module provides validation infrastructure for generated C++ code.
These validators are used by both the code generation pipeline and the CLI.

Validators:
- SyntaxValidator: Validates C++ syntax using Clang
- SemanticValidator: Cross-references code against SVD definitions
- CompileValidator: Full ARM GCC compilation test
- TestValidator: Auto-generates and runs unit tests

Usage (for CLI integration):
    from core.validators import (
        SyntaxValidator,
        SemanticValidator,
        CompileValidator,
        TestValidator
    )

    # In CLI's ValidationService
    class ValidationService:
        def __init__(self):
            self.syntax = SyntaxValidator()
            self.semantic = SemanticValidator()
            self.compile = CompileValidator()
            self.test = TestValidator()
"""

from .syntax_validator import SyntaxValidator
from .semantic_validator import SemanticValidator
from .compile_validator import CompileValidator
from .test_validator import TestValidator

__all__ = [
    "SyntaxValidator",
    "SemanticValidator",
    "CompileValidator",
    "TestValidator",
]
