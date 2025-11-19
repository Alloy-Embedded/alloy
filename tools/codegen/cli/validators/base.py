"""
Base classes for code validation framework.
"""

from abc import ABC, abstractmethod
from enum import Enum
from typing import List, Optional, Dict, Any
from pathlib import Path
from dataclasses import dataclass, field


class ValidationStage(str, Enum):
    """Validation stages."""
    SYNTAX = "syntax"
    SEMANTIC = "semantic"
    COMPILE = "compile"
    TEST = "test"


class ValidationSeverity(str, Enum):
    """Validation message severity."""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class ValidationMessage:
    """Individual validation message."""
    severity: ValidationSeverity
    stage: ValidationStage
    message: str
    file_path: Optional[Path] = None
    line: Optional[int] = None
    column: Optional[int] = None
    code_snippet: Optional[str] = None
    suggestion: Optional[str] = None

    def __str__(self):
        location = ""
        if self.file_path:
            location = f"{self.file_path}"
            if self.line:
                location += f":{self.line}"
                if self.column:
                    location += f":{self.column}"
            location += ": "

        return f"[{self.severity.value}] {location}{self.message}"


@dataclass
class ValidationResult:
    """Result of validation run."""
    stage: ValidationStage
    passed: bool = True
    messages: List[ValidationMessage] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    duration_ms: float = 0.0

    def add_error(
        self,
        message: str,
        file_path: Optional[Path] = None,
        line: Optional[int] = None,
        suggestion: Optional[str] = None
    ):
        """Add error message."""
        self.messages.append(ValidationMessage(
            severity=ValidationSeverity.ERROR,
            stage=self.stage,
            message=message,
            file_path=file_path,
            line=line,
            suggestion=suggestion
        ))
        self.passed = False

    def add_warning(
        self,
        message: str,
        file_path: Optional[Path] = None,
        line: Optional[int] = None,
        suggestion: Optional[str] = None
    ):
        """Add warning message."""
        self.messages.append(ValidationMessage(
            severity=ValidationSeverity.WARNING,
            stage=self.stage,
            message=message,
            file_path=file_path,
            line=line,
            suggestion=suggestion
        ))

    def add_info(
        self,
        message: str,
        file_path: Optional[Path] = None
    ):
        """Add info message."""
        self.messages.append(ValidationMessage(
            severity=ValidationSeverity.INFO,
            stage=self.stage,
            message=message,
            file_path=file_path
        ))

    def error_count(self) -> int:
        """Count error messages."""
        return sum(1 for m in self.messages if m.severity == ValidationSeverity.ERROR)

    def warning_count(self) -> int:
        """Count warning messages."""
        return sum(1 for m in self.messages if m.severity == ValidationSeverity.WARNING)

    def has_errors(self) -> bool:
        """Check if validation has errors."""
        return not self.passed


class Validator(ABC):
    """Base class for all validators."""

    def __init__(self):
        self.stage = ValidationStage.SYNTAX  # Override in subclasses

    @abstractmethod
    def validate(self, file_path: Path, **options) -> ValidationResult:
        """
        Validate a file.

        Args:
            file_path: Path to file to validate
            **options: Validator-specific options

        Returns:
            ValidationResult with messages
        """
        pass

    @abstractmethod
    def is_available(self) -> bool:
        """
        Check if validator is available (tools installed, etc.).

        Returns:
            True if validator can run, False otherwise
        """
        pass

    def get_name(self) -> str:
        """Get validator name."""
        return self.__class__.__name__
